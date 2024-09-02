#pragma once

enum mstage_e {
  ms_detecting = 0,
  ms_preparing = 1,
  ms_measuring = 2,
  ms_analyzing = 3,
  ms_result = 4,
  ms_stop = 5,
  ms_last
};

enum mmode_e {
  mm_detecting = 0,
  mm_fast = 1,
  mm_continuous = 2,
  mm_last
};

enum channel_e {
  ch_detecting = 0,
  ch_internal = 1,
  ch_external = 2,
  ch_last
};

enum datatype_e {
  dt_ecgdata = 1,
  dt_ecgresult = 2,
  dt_last
};

struct dataset {
  uint16_t energy;
  uint16_t volume;
  uint8_t gain;
  enum mstage_e mstage;
  enum mmode_e mmode;
  enum channel_e channel;
  enum datatype_e datatype;
  bool leadoff;
  uint8_t heartrate;
  uint8_t rssi;
#define DYNSIZE (offsetof(struct dataset, lbatt) - offsetof(struct dataset, energy))
  uint8_t lbatt;
  uint8_t rbatt;
  bool overrun;
  bool underrun;
} __attribute__((packed));

extern void dataSend(struct dataset ds, int num, int8_t *samples);
extern void rbattSend(uint8_t batt);
extern void lbattSend(uint8_t batt);
extern void dataFetch(struct dataset *ds_p, int num, int8_t *samples_p);