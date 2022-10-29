#ifndef STRUCT_GROUP_H
#define STRUCT_GROUP_H
#include "client2.h"
struct group
{ 
    int number_members;
    Client* group_members;
    char* group_name;
};
typedef struct group Group;

#endif