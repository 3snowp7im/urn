#ifndef __urn_h__
#define __urn_h__

#define URN_INFO_BEHIND_TIME   (1)
#define URN_INFO_LOSING_TIME   (2)
#define URN_INFO_BEST_SPLIT    (4)
#define URN_INFO_BEST_SEGMENT  (8)

struct urn_game {
    char *path;
    char *title;
    char *style;
    int width;
    int height;
    long long world_record;
    long long start_delay;
    char **split_titles;
    int split_count;
    long long *split_times;
    long long *segment_times;
    long long *best_splits;
    long long *best_segments;
};
typedef struct urn_game urn_game;

struct urn_timer {
    int running;
    long long now;
    long long start_time;
    long long time;
    long long sum_of_bests;
    int curr_split;
    long long *split_times;
    long long *split_deltas;
    long long *segment_times;
    long long *segment_deltas;
    int *split_info;
    long long *best_splits;
    long long *best_segments;
    const urn_game *game;
};
typedef struct urn_timer urn_timer;

long long urn_time_now(void);

long long urn_time_value(const char *string);

void urn_time_string(char *string, long long time);

void urn_time_millis_string(char *seconds, char *millis, long long time);

void urn_split_string(char *string, long long time);

void urn_delta_string(char *string, long long time);

int urn_game_create(urn_game **game_ptr, const char *path);

void urn_game_update_splits(urn_game *game, const urn_timer *timer);

void urn_game_update_bests(urn_game *game, const urn_timer *timer);

int urn_game_save(const urn_game *game);

void urn_game_release(urn_game *game);

int urn_timer_create(urn_timer **timer_ptr, const urn_game *game);

void urn_timer_release(urn_timer *timer);

int urn_timer_start(urn_timer *timer);

void urn_timer_step(urn_timer *timer, long long now);

int urn_timer_split(urn_timer *timer);

void urn_timer_stop(urn_timer *timer);

void urn_timer_reset(urn_timer *timer);

#endif
