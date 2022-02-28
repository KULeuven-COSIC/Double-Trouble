#ifndef CAT_H
#define CAT_H

int cache_allocate(int core, uint64_t mask);

int cat_set_ways(int core, uint64_t  mask);
int cat_get_ways(int core, uint64_t* mask);

#endif