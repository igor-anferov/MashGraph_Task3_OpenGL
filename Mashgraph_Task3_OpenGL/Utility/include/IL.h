#ifndef IL_h
#define IL_h

#include "IL/il.h"
#include "IL/ilu.h"

void IL_init() {
    ilInit();
    iluInit();
    
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
    
    ilEnable(IL_FORMAT_SET);
    ilSetInteger(IL_FORMAT_MODE,IL_RGBA);
    
    ilEnable(IL_TYPE_SET);
    ilSetInteger(IL_TYPE_MODE,IL_UNSIGNED_BYTE);
}

#endif /* IL_h */
