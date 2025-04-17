#include "internal.h"

const float default_speaker_positions[][3] =
  { 
    {-2.0, +0.0, +1.0}, // Left front bottom
    {+2.0, +0.0, +1.0}, // Right front bottom
    {-2.0, +0.0, -1.0}, // Left rear bottom
    {+2.0, +0.0, -1.0}, // Right rear bottom
    {+0.0, +0.0, +1.0}, // Center front
    {+0.0, +0.0, +0.0}, // LFE
    {-2.0, +0.0, +0.0}, // Left side
    {+2.0, +0.0, +0.0}, // Right side
    {-2.0, +1.0, +1.0}, // Left front top
    {+2.0, +1.0, +1.0}, // Right front top
    {-2.0, +1.0, -1.0}, // Left rear top
    {+2.0, +1.0, -1.0}, // Right rear top
    {+0.0, +0.0, -1.0}, // Center rear
    {-2.0, +1.0, +0.0}, // Left side Top
    {+2.0, +1.0, +0.0}, // Right side top
    {+0.0, +0.0, +0.0}, // LFE2
    {+4.0, +0.0, +4.0}, // Left front wide
    {-4.0, +0.0, +4.0}, // Right front wide
    {+0.0, +1.0, +0.0}, // Center top
    {+0.0, +1.0, +1.0}, // Center front top
    {+0.0, +1.0, -1.0}, // Center rear top
    {-2.0, +0.0, -1.0}, // Left rear center
    {+2.0, +0.0, -1.0}, // Right rear center
    {-2.0, +0.0, +0.0}, // LFE Left
    {+2.0, +0.0, +0.0}, // LFE Right
    {+1.0, +2.0, +1.0}, // Left front high
    {-1.0, +2.0, +1.0}, // Right front high
    {+0.0, +2.0, +1.0}, // Center front high
    {+0.0, -1.0, +0.0}, // Center bottom
    {-2.0, -1.0, +0.0}, // Left center bottom
    {+2.0, -1.0, +0.0}, // Right center bottom
    {+1.0, +1.0, +0.0}, // Left front center top
    {-1.0, -1.0, +0.0}, // Right front center bottom
  };

MIXED_EXPORT int mixed_default_speaker_position(float position[3], mixed_channel_t channel){
  if(channel < MIXED_MAX_SPEAKER_COUNT){
    memcpy(position, default_speaker_positions[channel], sizeof(float)*3);
    return 1;
  }
  mixed_err(MIXED_BAD_CHANNEL_CONFIGURATION);
  return 0;
}

const struct mixed_channel_configuration default_configurations[] =
  {
    /* 0.0 */ {0, {0}},
    /* 1.0 */ {1, {MIXED_MONO}},
    /* 2.0 */ {2, {MIXED_LEFT, MIXED_RIGHT}},
    /* 3.0 */ {3, {MIXED_LEFT, MIXED_RIGHT, MIXED_CENTER}},
    /* 4.0 */ {4, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_LEFT_REAR, MIXED_RIGHT_REAR}},
    /* 5.0 */ {5, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_LEFT_REAR, MIXED_RIGHT_REAR}},
    /* 5.1 */ {6, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_SUBWOOFER, MIXED_LEFT_REAR, MIXED_RIGHT_REAR}},
    /* 6.1 */ {7, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_SUBWOOFER, MIXED_CENTER_REAR, MIXED_LEFT_SIDE, MIXED_RIGHT_SIDE}},
    /* 7.1 */ {8, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_SUBWOOFER, MIXED_LEFT_REAR, MIXED_RIGHT_REAR, MIXED_LEFT_SIDE, MIXED_RIGHT_SIDE}},
    /* 9.0 */ {9, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_LEFT_REAR, MIXED_RIGHT_REAR, MIXED_LEFT_SIDE, MIXED_RIGHT_SIDE, MIXED_LEFT_FRONT_TOP, MIXED_RIGHT_FRONT_TOP}},
    /* 9.1 */ {10, {MIXED_LEFT_FRONT, MIXED_RIGHT_FRONT, MIXED_CENTER, MIXED_SUBWOOFER, MIXED_LEFT_REAR, MIXED_RIGHT_REAR, MIXED_LEFT_SIDE, MIXED_RIGHT_SIDE, MIXED_LEFT_FRONT_TOP, MIXED_RIGHT_FRONT_TOP}},
  };

MIXED_EXPORT struct mixed_channel_configuration const* mixed_default_channel_configuration(mixed_channel_t channel_count){
  if(0 < channel_count && channel_count < 11){
    return &default_configurations[channel_count];
  }
  mixed_err(MIXED_BAD_CHANNEL_CONFIGURATION);
  return 0;
}

MIXED_EXPORT int mixed_configuration_is_surround(struct mixed_channel_configuration const* configuration){
  /// Check if we have any side or back channels.
  for(mixed_channel_t i=0; i<configuration->count; ++i){
    switch(configuration->positions[i]){
    case MIXED_LEFT_REAR_BOTTOM:
    case MIXED_RIGHT_REAR_BOTTOM:
    case MIXED_LEFT_SIDE:
    case MIXED_RIGHT_SIDE:
    case MIXED_LEFT_REAR_TOP:
    case MIXED_RIGHT_REAR_TOP:
    case MIXED_CENTER_REAR:
    case MIXED_LEFT_SIDE_TOP:
    case MIXED_RIGHT_SIDE_TOP:
    case MIXED_CENTER_TOP:
    case MIXED_CENTER_REAR_TOP:
    case MIXED_LEFT_REAR_CENTER:
    case MIXED_RIGHT_REAR_CENTER:
    case MIXED_SUBWOOFER_LEFT:
    case MIXED_SUBWOOFER_RIGHT:
      return 1;
    }
  }
  return 0;
}

//// This is an implementation of "Vector Based Amplitude Panning" algorithms
//// as described by Ville Pulkki in "Spatial Sound Generation and Perception by Amplitude Panning Techniques"

/// The compute_gains function is used at runtime, regardless of data dimensionality.
VECTORIZE int mixed_compute_gains(const float position[3], float gains[], mixed_channel_t speakers[], mixed_channel_t *count, struct vbap_data *data){
  char dims = data->dims;
  float best_gain = -INFINITY;
  char best_negs = dims;
  mixed_channel_t best_set = 0;
  
  // If the direction is a zero vector, recast as 0,-1,0 on 3d, and 0,0,1 on 2d.
  // which should give a best approximation for being "on" the position.
  // Ideally we'd drive *all* speakers in that case, but the exact power of them
  // isn't clear to me.
  float direction[3];
  vec_normalize(direction, position);
  if(direction[0] == 0.0 && direction[1] == 0.0 && direction[2] == 0.0){
    if(dims == 3) direction[1] = -1.0;
    else          direction[2] = +1.0;
  }
  // Find set with the highest correspondence to our direction
  for(int i=0; i<data->set_count; ++i){
    float gain[3] = {0.0, 0.0, 0.0};
    float cur_gain = INFINITY;
    char cur_negs = dims;
    
    for(int j=0; j<dims; ++j){
      gain[j] = direction[0] * data->sets[i].inv_mat[0+j*3] +
                direction[1] * data->sets[i].inv_mat[1+j*3] +
                direction[2] * data->sets[i].inv_mat[2+j*3];
      if(gain[j] < cur_gain)
        cur_gain = gain[j];
      if(-0.01 <= gain[j])
        cur_negs--;
    }
    if(best_gain < cur_gain && cur_negs <= best_negs){
      best_gain = cur_gain;
      best_negs = cur_negs;
      best_set = i;
      memcpy(gains, gain, sizeof(float)*3);
    }
  }
  memcpy(speakers, data->sets[best_set].speakers, sizeof(mixed_channel_t)*3);
  // Handle if the best set still does not contain our point.
  char okey = 1;
  for(int i=0; i<dims; ++i){
    if(gains[i] < -0.01){
      gains[i] = 0.0001;
      okey = 0;
    }
  }
  vec_normalize(gains, gains);
  *count = dims;
  return okey;
}

/// Helper functions for 3D speaker set triangulation
float set_side_length(mixed_channel_t set, struct vbap_data *data){
  float cross[3];
  float *i = data->speakers[data->sets[set].speakers[0]];
  float *j = data->speakers[data->sets[set].speakers[1]];
  float *k = data->speakers[data->sets[set].speakers[2]];
  float length = fabsf(vec_angle(i, j))
                 + fabsf(vec_angle(i, k))
                 + fabsf(vec_angle(j, k));
  vec_cross(cross, i, j);
  if(0.000001 < length) return fabsf(vec_dot(cross, k)) / length;
  return 0.0;
}

int lines_intersect(int _i, int _j, int _k, int _l, struct vbap_data *data){
  float v1[3], v2[3], v3[3], _v3[3];
  float *i = data->speakers[_i];
  float *j = data->speakers[_j];
  float *k = data->speakers[_k];
  float *l = data->speakers[_l];
  vec_cross(v1, i, j);
  vec_cross(v2, k, l);
  vec_cross(v3, v1, v2);
  _v3[0] = -v3[0]; _v3[1] = -v3[1]; _v3[2] = -v3[2];

  float ij = vec_angle(i, j), kl = vec_angle(k, l);
  float i3 = vec_angle(i, v3), j3 = vec_angle(j, v3);
  float k3 = vec_angle(k, v3), l3 = vec_angle(l, v3);
  float i_3 = vec_angle(i, _v3), j_3 = vec_angle(j, _v3);
  float k_3 = vec_angle(k, _v3), l_3 = vec_angle(l, _v3);

  if(fabsf(i3) <= 0.01 || fabsf(j3) <= 0.01 ||
     fabsf(k3) <= 0.01 || fabsf(l3) <= 0.01 ||
     fabsf(i_3) <= 0.01 || fabsf(j_3) <= 0.01 ||
     fabsf(k_3) <= 0.01 || fabsf(l_3) <= 0.01)
    return 0;
  
  return (fabsf(ij - (i3 + j3)) <= 0.01 && fabsf(kl - (k3 + l3)) <= 0.01) ||
         (fabsf(ij - (i_3 + j_3)) <= 0.01 && fabsf(kl - (k_3 + l_3)) <= 0.01);
}

void compute_set_matrix_3d(int set, struct vbap_data *data){
  int i = data->sets[set].speakers[0];
  int j = data->sets[set].speakers[1];
  int k = data->sets[set].speakers[2];
  float *ip = data->speakers[i];
  float *jp = data->speakers[j];
  float *kp = data->speakers[k];

  float inv_det = 1.0 / (ip[0] * (jp[1] * kp[2] - jp[2] * kp[1]) -
                         ip[1] * (jp[0] * kp[2] - jp[2] * kp[0]) +
                         ip[2] * (jp[0] * kp[1] - jp[1] * kp[0]));

  data->sets[set].inv_mat[0] = (jp[1] * kp[2] - jp[2] * kp[1]) * +inv_det;
  data->sets[set].inv_mat[1] = (jp[0] * kp[2] - jp[2] * kp[0]) * -inv_det;
  data->sets[set].inv_mat[2] = (jp[0] * kp[1] - jp[1] * kp[0]) * +inv_det;
  data->sets[set].inv_mat[3] = (ip[1] * kp[2] - ip[2] * kp[1]) * -inv_det;
  data->sets[set].inv_mat[4] = (ip[0] * kp[2] - ip[2] * kp[0]) * +inv_det;
  data->sets[set].inv_mat[5] = (ip[0] * kp[1] - ip[1] * kp[0]) * -inv_det;
  data->sets[set].inv_mat[6] = (ip[1] * jp[2] - ip[2] * jp[1]) * +inv_det;
  data->sets[set].inv_mat[7] = (ip[0] * jp[2] - ip[2] * jp[0]) * -inv_det;
  data->sets[set].inv_mat[8] = (ip[0] * jp[1] - ip[1] * jp[0]) * +inv_det;
}

int speakers_within_set(int set, struct vbap_data *data){
  compute_set_matrix_3d(set, data);
  float *inv_mat = data->sets[set].inv_mat;
  int i = data->sets[set].speakers[0];
  int j = data->sets[set].speakers[1];
  int k = data->sets[set].speakers[2];
  for(int s=0; s<data->speaker_count; ++s){
    if(s != i && s != j && s != k){
      float *sp = data->speakers[s];
      char inside = 1;
      for(int t=0; t<3; ++t){
        float tmp = sp[0] * inv_mat[0 + t*3] +
                    sp[1] * inv_mat[1 + t*3] +
                    sp[2] * inv_mat[2 + t*3];
        if(tmp < -0.001) inside = 0;
      }
      if(inside) return 1;
    }
  }
  return 0;
}

int make_vbap_3d(struct vbap_data *data){
  mixed_channel_t speaker_count = data->speaker_count;
  int set_count = 0;
  
  // Compute the speaker sets. We start by eagerly constructing all possible triangle combinations.
  char connected[MIXED_MAX_SPEAKER_COUNT][MIXED_MAX_SPEAKER_COUNT] = {0};
  for(int i=0; i<speaker_count; ++i){
    for(int j=i+1; j<speaker_count; ++j){
      for(int k=j+1; k<speaker_count; ++k){
        data->sets[set_count].speakers[0] = i;
        data->sets[set_count].speakers[1] = j;
        data->sets[set_count].speakers[2] = k;
        if(0.01 < set_side_length(set_count, data)){
          connected[i][j] = 1; connected[j][i] = 1;
          connected[i][k] = 1; connected[k][i] = 1;
          connected[j][k] = 1; connected[k][j] = 1;
          ++set_count;
        }
      }
    }
  }

  // Next we compute the distances between all speakers and sort them via a simple
  // insertion sort.
  float distances[MAX_VBAP_SETS];
  int distances_i[MAX_VBAP_SETS];
  int distances_j[MAX_VBAP_SETS];
  for(int i=0; i<set_count; ++i){
    distances[i] = INFINITY;
  }
  for(int i=0; i<speaker_count; ++i){
    for(int j=i+1; j<speaker_count; ++j){
      if(connected[i][j]){
        float dist = fabs(vec_angle(data->speakers[i], data->speakers[j]));
        int k=0; while(distances[k] < dist) ++k;
        for(int l=set_count-1; k < l; --l){
          distances[l] = distances[l-1];
          distances_i[l] = distances_i[l-1];
          distances_j[l] = distances_j[l-1];
        }
        distances[k] = dist;
        distances_i[k] = i;
        distances_j[k] = j;
      }
    }
  }

  // Disconnect connections which cross over ones that are shorter.
  for(int i=0; i<set_count; ++i){
    int first = distances_i[i];
    int second = distances_j[i];
    if(connected[first][second]){
      for(int j=0; j<speaker_count; ++j){
        if(j != first && j != second){
          for(int k=j+1; k<speaker_count; ++k){
            if(k != first && k != second && lines_intersect(first, second, j, k, data)){
              connected[j][k] = 0;
              connected[k][j] = 0;
            }
          }
        }
      }
    }
  }

  // Remove all sets that cross lines or include speakers.
  for(int s=0; s<set_count; ++s){
    int i = data->sets[s].speakers[0];
    int j = data->sets[s].speakers[1];
    int k = data->sets[s].speakers[2];
    if(!connected[i][j] || !connected[i][k] || !connected[j][k] ||
       speakers_within_set(s, data)){
      // Shift sets downwards
      for(int t=s; t<set_count-1; ++t){
        data->sets[t] = data->sets[t+1];
        --set_count;
      }
    }
  }

  for(int s=0; s<set_count; ++s){
    compute_set_matrix_3d(s, data);
  }

  if(set_count == 0){
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  }
  
  data->set_count = set_count;
  return 1;
}

/// Helpers for 2D speaker set division
#define RAD2DEG 360.0/(2.0f*M_PI)
#define DEG2RAD (2.0f*M_PI)/360.0

int compute_set_matrix_2d(int set, float azimuth[], struct vbap_data *data){
  int i = data->sets[set].speakers[0];
  int j = data->sets[set].speakers[1];
  // TODO: I feel like it would be better to extract these from
  //       the positions instead of doing more trig, but I'll
  //       leave that for when it's actually tested.
  float x1 = cos(azimuth[i] * DEG2RAD);
  float x2 = sin(azimuth[i] * DEG2RAD);
  float x3 = cos(azimuth[j] * DEG2RAD);
  float x4 = sin(azimuth[j] * DEG2RAD);
  float det = (x1 * x4 - x3 * x2);

  memset(data->sets[set].inv_mat, 0, sizeof(float)*9);
  if(fabsf(det) <= 0.001){
    return 0;
  }
  
  float inv_det = 1.0 / det;
  // The matrix is constructed in such a way that the Y component of the input
  // vector is zeroed on multiplication. The matrix is in row-major format.
  data->sets[set].inv_mat[0] = x4 * +inv_det;
  data->sets[set].inv_mat[2] = x3 * -inv_det;
  data->sets[set].inv_mat[3] = x2 * -inv_det;
  data->sets[set].inv_mat[5] = x1 * +inv_det;
  return 1;
}

int make_vbap_2d(struct vbap_data *data){
  mixed_channel_t speaker_count = data->speaker_count;
  int set_count = 0;

  // Get azimuth angles in [-180, +180]
  float azimuth[MIXED_MAX_SPEAKER_COUNT];
  for(int i=0; i<speaker_count; ++i){
    azimuth[i] = acos(data->speakers[i][0]) * RAD2DEG;
    if(0.001 < fabs(data->speakers[i][2])){
      azimuth[i] *= data->speakers[i][2] / fabs(data->speakers[i][2]);
    }
  }
  
  // Sort the speakers by angle
  int sorted[MIXED_MAX_SPEAKER_COUNT];
  for(int i=0; i<speaker_count; ++i){
    float smallest_angle = 1024.0;
    int smallest = 0;
    for(int j=0; j<speaker_count; ++j){
      if(azimuth[j] < smallest_angle){
        smallest_angle = azimuth[j];
        smallest = j;
      }
    }
    sorted[i] = smallest;
    // Bias this one to not get picked again.
    azimuth[smallest] += 2048.0;
  }
  // Undo the bias.
  for(int i=0; i<speaker_count; ++i){
    azimuth[i] -= 2048.0;
  }

  // Construct pairs based on adjacent speakers
  for(int i=0; i<speaker_count-1; ++i){
    if(azimuth[i+1] - azimuth[i] <= (170)){
      data->sets[set_count].speakers[0] = sorted[i];
      data->sets[set_count].speakers[1] = sorted[i+1];
      data->sets[set_count].speakers[2] = -1;
      if(compute_set_matrix_2d(set_count, azimuth, data)){
        ++set_count;
      }
    }
  }
  if(360 - azimuth[sorted[speaker_count-1]] + azimuth[sorted[0]] <= 170){
      data->sets[set_count].speakers[0] = sorted[speaker_count-1];
      data->sets[set_count].speakers[1] = sorted[0];
      data->sets[set_count].speakers[2] = -1;
      if(compute_set_matrix_2d(set_count, azimuth, data)){
        ++set_count;
      }
  }

  if(set_count == 0){
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  }
  
  data->set_count = set_count;
  return 1;
}

int make_vbap(float speakers[][3], mixed_channel_t speaker_count, int dim, struct vbap_data *data){
  if(speaker_count < dim || MIXED_MAX_SPEAKER_COUNT < speaker_count){
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  }

  data->dims = dim;
  data->speaker_count = speaker_count;
  switch(dim){
  case 2:
    memcpy(data->speakers, speakers, speaker_count*3*sizeof(float));
    for(mixed_channel_t i=0; i<speaker_count; ++i){
      // Clamp Y to 0 to make the vectors planar in XZ.
      data->speakers[i][1] = 0.0;
      vec_normalized(data->speakers[i]);
    }
    return make_vbap_2d(data);
  case 3:
    for(mixed_channel_t i=0; i<speaker_count; ++i){
      vec_normalize(data->speakers[i], speakers[i]);
    }
    return make_vbap_3d(data);
  default:
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  }
}

int make_vbap_from_configuration(struct mixed_channel_configuration const* configuration, struct vbap_data *data){
  if(configuration->count < 2){
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  }
  // Get default plane
  float speakers[configuration->count][3];
  if(!mixed_default_speaker_position(speakers[0], configuration->positions[0])) return 0;
  float plane = speakers[0][1];
  // Initialise rest of the speakers and check if we're actually 3D or not.
  int dim = 2;
  for(int i=1; i<configuration->count; ++i){
    if(!mixed_default_speaker_position(speakers[i], configuration->positions[i])) return 0;
    if(speakers[i][1] != plane) dim = 3;
  }
  return make_vbap(speakers, configuration->count, dim, data);
}

int make_vbap_from_channel_count(mixed_channel_t speaker_count, struct vbap_data *data){
  struct mixed_channel_configuration const* configuration = mixed_default_channel_configuration(speaker_count);
  if(!configuration) return 0;
  return make_vbap_from_configuration(configuration, data);
}
