#include "../internal.h"

// Adapted from https://github.com/velipso/sndfilter/blob/master/src/compressor.c

#define MIXED_COMPRESSOR_MAXDELAY 1024
#define MIXED_COMPRESSOR_SAMPLES_PER_UPDATE 32
#define MIXED_COMPRESSOR_SPACING 5.0f

struct compressor_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  // user can read the metergain data variable after processing a chunk to see how much dB the
  // compressor would have liked to compress the sample; the meter values aren't used to shape the
  // sound in any way, only used for output if desired
  float metergain;
  // everything else shouldn't really be mucked with unless you read the algorithm and feel
  // comfortable
  float meterrelease;
  float threshold;
  float knee;
  float linearpregain;
  float linearthreshold;
  float slope;
  float attacksamplesinv;
  float satreleasesamplesinv;
  float wet;
  float dry;
  float k;
  float kneedboffset;
  float linearthresholdknee;
  float mastergain;
  float a; // adaptive release polynomial coefficients
  float b;
  float c;
  float d;
  float detectoravg;
  float compgain;
  float maxcompdiffdb;
  uint32_t delaybufsize;
  uint32_t delaywritepos;
  uint32_t delayreadpos;
  float delaybuf[MIXED_COMPRESSOR_MAXDELAY];

  // Raw settings
  uint32_t samplerate;
  float pregain;
  float ratio;
  float attack;
  float release;
  float predelay;
  float releasezone[4];
  float postgain;
};

static inline float adaptivereleasecurve(float x, float a, float b, float c, float d){
  // a*x^3 + b*x^2 + c*x + d
  float x2 = x * x;
  return a * x2 * x + b * x2 + c * x + d;
}

static inline float clampf(float v, float min, float max){
  return v < min ? min : (v > max ? max : v);
}

static inline float absf(float v){
  return v < 0.0f ? -v : v;
}

static inline float fixf(float v, float def){
  // fix NaN and infinity values that sneak in... not sure why this is needed, but it is
  if (isnan(v) || isinf(v))
    return def;
  return v;
}

static inline float db2lin(float db){
  return powf(10.0f, 0.05f * db);
}

static inline float lin2db(float lin){
  return 20.0f * log10f(lin);
}

static inline float kneecurve(float x, float k, float linearthreshold){
  return linearthreshold + (1.0f - expf(-k * (x - linearthreshold))) / k;
}

static inline float kneeslope(float x, float k, float linearthreshold){
  return k * x / ((k * linearthreshold + 1.0f) * expf(k * (x - linearthreshold)) - 1);
}

static inline float compcurve(float x, float k, float slope, float linearthreshold, float linearthresholdknee, float threshold, float knee, float kneedboffset){
  if (x < linearthreshold)
    return x;
  if (knee <= 0.0f) // no knee in curve
    return db2lin(threshold + slope * (lin2db(x) - threshold));
  if (x < linearthresholdknee)
    return kneecurve(x, k, linearthreshold);
  return db2lin(kneedboffset + slope * (lin2db(x) - threshold - knee));
}

VECTORIZE int compressor_segment_mix(struct mixed_segment *segment){
  struct compressor_segment_data *data = (struct compressor_segment_data *)segment->data;

  float metergain            = data->metergain;
  float meterrelease         = data->meterrelease;
  float threshold            = data->threshold;
  float knee                 = data->knee;
  float linearpregain        = data->linearpregain;
  float linearthreshold      = data->linearthreshold;
  float slope                = data->slope;
  float attacksamplesinv     = data->attacksamplesinv;
  float satreleasesamplesinv = data->satreleasesamplesinv;
  float wet                  = data->wet;
  float dry                  = data->dry;
  float k                    = data->k;
  float kneedboffset         = data->kneedboffset;
  float linearthresholdknee  = data->linearthresholdknee;
  float mastergain           = data->mastergain;
  float a                    = data->a;
  float b                    = data->b;
  float c                    = data->c;
  float d                    = data->d;
  float detectoravg          = data->detectoravg;
  float compgain             = data->compgain;
  float maxcompdiffdb        = data->maxcompdiffdb;
  int delaybufsize           = data->delaybufsize;
  int delaywritepos          = data->delaywritepos;
  int delayreadpos           = data->delayreadpos;
  float *delaybuf            = data->delaybuf;

  int samplesperchunk = MIXED_COMPRESSOR_SAMPLES_PER_UPDATE;
  float ang90 = (float)M_PI * 0.5f;
  float ang90inv = 2.0f / (float)M_PI;
  int samplepos = 0;
  float spacingdb = MIXED_COMPRESSOR_SPACING;

  uint32_t samples = UINT32_MAX;
  float *input, *output;
  mixed_buffer_request_read(&input, &samples, data->in);
  mixed_buffer_request_write(&output, &samples, data->out);
  
  int chunks = samples / samplesperchunk;
  samples = chunks * samplesperchunk;

  for (int ch = 0; ch < chunks; ch++){
    detectoravg = fixf(detectoravg, 1.0f);
    float desiredgain = detectoravg;
    float scaleddesiredgain = asinf(desiredgain) * ang90inv;
    float compdiffdb = lin2db(compgain / scaleddesiredgain);

    // calculate envelope rate based on whether we're attacking or releasing
    float enveloperate;
    if (compdiffdb < 0.0f){ // compgain < scaleddesiredgain, so we're releasing
      compdiffdb = fixf(compdiffdb, -1.0f);
      maxcompdiffdb = -1; // reset for a future attack mode
      // apply the adaptive release curve
      // scale compdiffdb between 0-3
      float x = (clampf(compdiffdb, -12.0f, 0.0f) + 12.0f) * 0.25f;
      float releasesamples = adaptivereleasecurve(x, a, b, c, d);
      enveloperate = db2lin(spacingdb / releasesamples);
    }
    else{ // compresorgain > scaleddesiredgain, so we're attacking
      compdiffdb = fixf(compdiffdb, 1.0f);
      if (maxcompdiffdb == -1 || maxcompdiffdb < compdiffdb)
        maxcompdiffdb = compdiffdb;
      float attenuate = maxcompdiffdb;
      if (attenuate < 0.5f)
        attenuate = 0.5f;
      enveloperate = 1.0f - powf(0.25f / attenuate, attacksamplesinv);
    }

    // process the chunk
    for (int chi = 0; chi < samplesperchunk; chi++, samplepos++,
           delayreadpos = (delayreadpos + 1) % delaybufsize,
           delaywritepos = (delaywritepos + 1) % delaybufsize){

      float inputL = input[samplepos] * linearpregain;
      delaybuf[delaywritepos] = inputL;

      inputL = absf(inputL);

      float attenuation;
      if (inputL < 0.0001f)
        attenuation = 1.0f;
      else{
        float inputcomp = compcurve(inputL, k, slope, linearthreshold, linearthresholdknee, threshold, knee, kneedboffset);
        attenuation = inputcomp / inputL;
      }

      float rate;
      if (attenuation > detectoravg){ // if releasing
        float attenuationdb = -lin2db(attenuation);
        if (attenuationdb < 2.0f)
          attenuationdb = 2.0f;
        float dbpersample = attenuationdb * satreleasesamplesinv;
        rate = db2lin(dbpersample) - 1.0f;
      }
      else
        rate = 1.0f;

      detectoravg += (attenuation - detectoravg) * rate;
      if (detectoravg > 1.0f)
        detectoravg = 1.0f;
      detectoravg = fixf(detectoravg, 1.0f);

      if (enveloperate < 1) // attack, reduce gain
        compgain += (scaleddesiredgain - compgain) * enveloperate;
      else{ // release, increase gain
        compgain *= enveloperate;
        if (compgain > 1.0f)
          compgain = 1.0f;
      }

      // the final gain value!
      float premixgain = sinf(ang90 * compgain);
      float gain = dry + wet * mastergain * premixgain;

      // calculate metering (not used in core algo, but used to output a meter if desired)
      float premixgaindb = lin2db(premixgain);
      if (premixgaindb < metergain)
        metergain = premixgaindb; // spike immediately
      else
        metergain += (premixgaindb - metergain) * meterrelease; // fall slowly

      // apply the gain
      output[samplepos] = delaybuf[delayreadpos] * gain;
    }
  }

  mixed_buffer_finish_write(samples, data->out);
  mixed_buffer_finish_read(samples, data->in);

  data->metergain     = metergain;
  data->detectoravg   = detectoravg;
  data->compgain      = compgain;
  data->maxcompdiffdb = maxcompdiffdb;
  data->delaywritepos = delaywritepos;
  data->delayreadpos  = delayreadpos;
  
  return 1;
}

int compressor_segment_start(struct mixed_segment *segment){
  struct compressor_segment_data *data = (struct compressor_segment_data *)segment->data;
  data->metergain = 1.0;
  data->detectoravg = 0.0;
  data->compgain = 1.0;
  data->maxcompdiffdb = -1.0;
  data->delaywritepos = 0;
  data->delayreadpos = (data->delaybufsize > 1)? 1 : 0;
  return 1;
}

void compressor_reinit(struct compressor_segment_data *data){
  uint32_t rate = data->samplerate;
  // setup the predelay buffer
  int delaybufsize = rate * data->predelay;
  if (delaybufsize < 1){
    delaybufsize = 1;
  }else if (delaybufsize > MIXED_COMPRESSOR_MAXDELAY){
    delaybufsize = MIXED_COMPRESSOR_MAXDELAY;
  }
  memset(data->delaybuf, 0, sizeof(float) * delaybufsize);

  // useful values
  float linearpregain = db2lin(data->pregain);
  float linearthreshold = db2lin(data->threshold);
  float slope = 1.0f / data->ratio;
  float attacksamples = rate * data->attack;
  float attacksamplesinv = 1.0f / attacksamples;
  float releasesamples = rate * data->release;
  float satrelease = 0.0025f; // seconds
  float satreleasesamplesinv = 1.0f / ((float)rate * satrelease);
  float dry = 1.0f - data->wet;

  // metering values (not used in core algorithm, but used to output a meter if desired)
  float metergain = 1.0f; // gets overwritten immediately because gain will always be negative
  float meterfalloff = 0.325f; // seconds
  float meterrelease = 1.0f - expf(-1.0f / ((float)rate * meterfalloff));

  // calculate knee curve parameters
  float k = 5.0f; // initial guess
  float kneedboffset = 0.0f;
  float linearthresholdknee = 0.0f;
  if (data->knee > 0.0f){ // if a knee exists, search for a good k value
    float xknee = db2lin(data->threshold + data->knee);
    float mink = 0.1f;
    float maxk = 10000.0f;
    // search by comparing the knee slope at the current k guess, to the ideal slope
    for (int i = 0; i < 15; i++){
      if (kneeslope(xknee, k, linearthreshold) < slope)
        maxk = k;
      else
        mink = k;
      k = sqrtf(mink * maxk);
    }
    kneedboffset = lin2db(kneecurve(xknee, k, linearthreshold));
    linearthresholdknee = db2lin(data->threshold + data->knee);
  }

  // calculate a master gain based on what sounds good
  float fulllevel = compcurve(1.0f, k, slope, linearthreshold, linearthresholdknee,
                              data->threshold, data->knee, kneedboffset);
  float mastergain = db2lin(data->postgain) * powf(1.0f / fulllevel, 0.6f);

  // calculate the adaptive release curve parameters
  // solve a,b,c,d in `y = a*x^3 + b*x^2 + c*x + d`
  // interescting points (0, y1), (1, y2), (2, y3), (3, y4)
  float y1 = releasesamples * data->releasezone[0];
  float y2 = releasesamples * data->releasezone[1];
  float y3 = releasesamples * data->releasezone[2];
  float y4 = releasesamples * data->releasezone[3];
  float a = (-y1 + 3.0f * y2 - 3.0f * y3 + y4) / 6.0f;
  float b = y1 - 2.5f * y2 + 2.0f * y3 - 0.5f * y4;
  float c = (-11.0f * y1 + 18.0f * y2 - 9.0f * y3 + 2.0f * y4) / 6.0f;
  float d = y1;

  // save everything
  data->metergain            = metergain;
  data->meterrelease         = meterrelease;
  data->linearpregain        = linearpregain;
  data->linearthreshold      = linearthreshold;
  data->slope                = slope;
  data->attacksamplesinv     = attacksamplesinv;
  data->satreleasesamplesinv = satreleasesamplesinv;
  data->dry                  = dry;
  data->k                    = k;
  data->kneedboffset         = kneedboffset;
  data->linearthresholdknee  = linearthresholdknee;
  data->mastergain           = mastergain;
  data->a                    = a;
  data->b                    = b;
  data->c                    = c;
  data->d                    = d;
  data->detectoravg          = 0.0f;
  data->compgain             = 1.0f;
  data->maxcompdiffdb        = -1.0f;
  data->delaybufsize         = delaybufsize;
  data->delaywritepos        = 0;
  data->delayreadpos         = delaybufsize > 1 ? 1 : 0;
}

int compressor_segment_mix_bypass(struct mixed_segment *segment){
  struct compressor_segment_data *data = (struct compressor_segment_data *)segment->data;
  return mixed_buffer_transfer(data->in, data->out);
}

int compressor_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct compressor_segment_data *data = (struct compressor_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location == 0){
      data->in = (struct mixed_buffer *)buffer;
      return 1;
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int compressor_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct compressor_segment_data *data = (struct compressor_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location == 0){
      data->out = (struct mixed_buffer *)buffer;
      return 1;
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int compressor_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct compressor_segment_data *data = (struct compressor_segment_data *)segment->data;
  switch(field){
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == compressor_segment_mix_bypass); break;
  case MIXED_COMPRESSOR_PREGAIN: *((float *)value) = data->pregain; break;
  case MIXED_COMPRESSOR_THRESHOLD: *((float *)value) = data->threshold; break;
  case MIXED_COMPRESSOR_KNEE: *((float *)value) = data->knee; break;
  case MIXED_COMPRESSOR_RATIO: *((float *)value) = data->ratio; break;
  case MIXED_COMPRESSOR_ATTACK: *((float *)value) = data->attack; break;
  case MIXED_COMPRESSOR_RELEASE: *((float *)value) = data->release; break;
  case MIXED_COMPRESSOR_PREDELAY: *((float *)value) = data->predelay; break;
  case MIXED_COMPRESSOR_POSTGAIN: *((float *)value) = data->postgain; break;
  case MIXED_MIX: *((float *)value) = data->wet; break;
  case MIXED_COMPRESSOR_RELEASEZONE: {
    float *zone = (float *)value;
    zone[0] = data->releasezone[0];
    zone[1] = data->releasezone[1];
    zone[2] = data->releasezone[2];
    zone[3] = data->releasezone[3];
  } break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int compressor_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct compressor_segment_data *data = (struct compressor_segment_data *)segment->data;
  switch(field){
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = compressor_segment_mix_bypass;
    }else{
      segment->mix = compressor_segment_mix;
    }
    break;
  case MIXED_COMPRESSOR_PREGAIN: data->pregain = *(float *)value; break;
  case MIXED_COMPRESSOR_THRESHOLD: data->threshold = *(float *)value; break;
  case MIXED_COMPRESSOR_KNEE: data->knee = *(float *)value; break;
  case MIXED_COMPRESSOR_RATIO: data->ratio = *(float *)value; break;
  case MIXED_COMPRESSOR_ATTACK: data->attack = *(float *)value; break;
  case MIXED_COMPRESSOR_RELEASE: data->release = *(float *)value; break;
  case MIXED_COMPRESSOR_PREDELAY: data->predelay = *(float *)value; break;
  case MIXED_COMPRESSOR_POSTGAIN: data->postgain = *(float *)value; break;
  case MIXED_MIX: data->wet = *(float *)value; break;
  case MIXED_COMPRESSOR_RELEASEZONE: {
    float *parts = (float *)value;
    data->releasezone[0] = parts[0];
    data->releasezone[1] = parts[1];
    data->releasezone[2] = parts[2];
    data->releasezone[3] = parts[3];
  } break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  compressor_reinit(data);
  return 1;
}

int compressor_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  info->name = "compressor";
  info->description = "Dynamically compress the audio volume.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");

  set_info_field(field++, MIXED_COMPRESSOR_PREGAIN,
                 MIXED_DECIBEL_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The gain before compression, in dB.");

  set_info_field(field++, MIXED_COMPRESSOR_THRESHOLD,
                 MIXED_DECIBEL_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The threshold to activate compression, in dB.");

  set_info_field(field++, MIXED_COMPRESSOR_KNEE,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The compression knee.");

  set_info_field(field++, MIXED_COMPRESSOR_RATIO,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The compression ratio.");

  set_info_field(field++, MIXED_COMPRESSOR_ATTACK,
                 MIXED_DURATION_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The attack time in seconds.");

  set_info_field(field++, MIXED_COMPRESSOR_RELEASE,
                 MIXED_DURATION_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The release time in seconds.");

  set_info_field(field++, MIXED_COMPRESSOR_PREDELAY,
                 MIXED_DURATION_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The delay before compression, in seconds.");

  set_info_field(field++, MIXED_COMPRESSOR_RELEASEZONE,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The release zones of the compressor.");

  set_info_field(field++, MIXED_COMPRESSOR_POSTGAIN,
                 MIXED_DECIBEL_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The gain after compressin, in dB.");

  set_info_field(field++, MIXED_MIX,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The dry/wet mix");

  set_info_field(field++, MIXED_COMPRESSOR_GAIN,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_GET,
                 "Retrieve the actual gain that was set during the last mixing step.");

  clear_info_field(field++);
  return 1;
}

MIXED_EXPORT int mixed_make_segment_compressor(uint32_t samplerate, struct mixed_segment *segment){
  struct compressor_segment_data *data = mixed_calloc(1, sizeof(struct compressor_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->samplerate = samplerate;
  data->pregain = 0.0;
  data->threshold = -24;
  data->knee = 30;
  data->ratio = 12;
  data->attack = 0.003;
  data->release = 0.25;
  data->predelay = 0.006;
  data->releasezone[0] = 0.09;
  data->releasezone[1] = 0.16;
  data->releasezone[2] = 0.42;
  data->releasezone[3] = 0.96;
  data->postgain = 0;
  data->wet = 1;

  compressor_reinit(data);
  
  segment->start = compressor_segment_start;
  segment->mix = compressor_segment_mix;
  segment->set_in = compressor_segment_set_in;
  segment->set_out = compressor_segment_set_out;
  segment->info = compressor_segment_info;
  segment->get = compressor_segment_get;
  segment->set = compressor_segment_set;
  segment->data = data;
  return 1;
}

int __make_compressor(void *args, struct mixed_segment *segment){
  return mixed_make_segment_compressor(ARG(uint32_t, 0), segment);
}

REGISTER_SEGMENT(compressor, __make_compressor, 1, {
    {.description = "samplerate", .type = MIXED_UINT32}})
