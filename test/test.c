#include "metaBuffer.h"

int main()
{
    metaBuffer _p;
    int ret = -1;

    memset(&_p, 0, sizeof(_p));
    _p.buffer = malloc(2048);
    _p.size = 2048;

    do 
    {
        ret = metabInit(&_p);
        if (ret == -1) 
            break;
   
        //metabFree(&_p);
        metabDump(&_p);

#ifdef THREAD_SAFE
        metabIncRef(&_p);
#endif        
        metabDump(&_p);
        ret = metabSetToken(&_p, "hello", "world");
        metabDump(&_p);

        ret = metabSetInt32(&_p, "samples", 999);
        metabDump(&_p);
        metabSetInt32(&_p, "PT", 110);
        metabDump(&_p);
        metabSetInt32(&_p, "rtp-time", 20);
        metabDump(&_p);
        metabSetInt32(&_p, "ssrc", 0x12345678);
        metabDump(&_p);
        metabSetBool(&_p, "damaged", false);
        metabDump(&_p);
        metabSetBool(&_p, "M", true);
        metabDump(&_p);
        // TODO:
        // "timeUs" a int64 type
        _p.ts = 0x0FFFFFFFFLL;


        char *val1;
        int val2;
        metabGetToken(&_p, "hello", &val1);
        metabGetInt32(&_p, "samples", &val2);
        printf("test %s %d\n",  val1, val2);

#ifdef THREAD_SAFE
        metabDecRef(&_p);
#endif        
        metabDump(&_p);
#ifdef THREAD_SAFE
        metabDecRef(&_p);
#endif        
    }while(0);

    metabDump(&_p);
    return(ret);
}
