#ifndef API_CLIENT_H
#define API_CLIENT_H

extern volatile long currentVisitors;

void fetchVisitorCount();   // blocking but fast; keeps last value on failure

#endif