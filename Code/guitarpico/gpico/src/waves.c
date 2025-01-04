/* waves.c */

#include <stdint.h>

const int16_t table_sine[1024]={
    0,201,402,603,804,1005,1206,1407,1608,1809,2009,2210,2410,2611,2811,3012,
    3212,3412,3612,3811,4011,4210,4410,4609,4808,5007,5205,5404,5602,5800,5998,6195,
    6393,6590,6786,6983,7179,7375,7571,7767,7962,8157,8351,8545,8739,8933,9126,9319,
    9512,9704,9896,10087,10278,10469,10659,10849,11039,11228,11417,11605,11793,11980,12167,12353,
    12539,12725,12910,13094,13279,13462,13645,13828,14010,14191,14372,14553,14732,14912,15090,15269,
    15446,15623,15800,15976,16151,16325,16499,16673,16846,17018,17189,17360,17530,17700,17869,18037,
    18204,18371,18537,18703,18868,19032,19195,19357,19519,19680,19841,20000,20159,20317,20475,20631,
    20787,20942,21096,21250,21403,21554,21705,21856,22005,22154,22301,22448,22594,22739,22884,23027,
    23170,23311,23452,23592,23731,23870,24007,24143,24279,24413,24547,24680,24811,24942,25072,25201,
    25329,25456,25582,25708,25832,25955,26077,26198,26319,26438,26556,26674,26790,26905,27019,27133,
    27245,27356,27466,27575,27683,27790,27896,28001,28105,28208,28310,28411,28510,28609,28706,28803,
    28898,28992,29085,29177,29268,29358,29447,29534,29621,29706,29791,29874,29956,30037,30117,30195,
    30273,30349,30424,30498,30571,30643,30714,30783,30852,30919,30985,31050,31113,31176,31237,31297,
    31356,31414,31470,31526,31580,31633,31685,31736,31785,31833,31880,31926,31971,32014,32057,32098,
    32137,32176,32213,32250,32285,32318,32351,32382,32412,32441,32469,32495,32521,32545,32567,32589,
    32609,32628,32646,32663,32678,32692,32705,32717,32728,32737,32745,32752,32757,32761,32765,32766,
    32767,32766,32765,32761,32757,32752,32745,32737,32728,32717,32705,32692,32678,32663,32646,32628,
    32609,32589,32567,32545,32521,32495,32469,32441,32412,32382,32351,32318,32285,32250,32213,32176,
    32137,32098,32057,32014,31971,31926,31880,31833,31785,31736,31685,31633,31580,31526,31470,31414,
    31356,31297,31237,31176,31113,31050,30985,30919,30852,30783,30714,30643,30571,30498,30424,30349,
    30273,30195,30117,30037,29956,29874,29791,29706,29621,29534,29447,29358,29268,29177,29085,28992,
    28898,28803,28706,28609,28510,28411,28310,28208,28105,28001,27896,27790,27683,27575,27466,27356,
    27245,27133,27019,26905,26790,26674,26556,26438,26319,26198,26077,25955,25832,25708,25582,25456,
    25329,25201,25072,24942,24811,24680,24547,24413,24279,24143,24007,23870,23731,23592,23452,23311,
    23170,23027,22884,22739,22594,22448,22301,22154,22005,21856,21705,21554,21403,21250,21096,20942,
    20787,20631,20475,20317,20159,20000,19841,19680,19519,19357,19195,19032,18868,18703,18537,18371,
    18204,18037,17869,17700,17530,17360,17189,17018,16846,16673,16499,16325,16151,15976,15800,15623,
    15446,15269,15090,14912,14732,14553,14372,14191,14010,13828,13645,13462,13279,13094,12910,12725,
    12539,12353,12167,11980,11793,11605,11417,11228,11039,10849,10659,10469,10278,10087,9896,9704,
    9512,9319,9126,8933,8739,8545,8351,8157,7962,7767,7571,7375,7179,6983,6786,6590,
    6393,6195,5998,5800,5602,5404,5205,5007,4808,4609,4410,4210,4011,3811,3612,3412,
    3212,3012,2811,2611,2410,2210,2009,1809,1608,1407,1206,1005,804,603,402,201,
    0,-201,-402,-603,-804,-1005,-1206,-1407,-1608,-1809,-2009,-2210,-2410,-2611,-2811,-3012,
    -3212,-3412,-3612,-3811,-4011,-4210,-4410,-4609,-4808,-5007,-5205,-5404,-5602,-5800,-5998,-6195,
    -6393,-6590,-6786,-6983,-7179,-7375,-7571,-7767,-7962,-8157,-8351,-8545,-8739,-8933,-9126,-9319,
    -9512,-9704,-9896,-10087,-10278,-10469,-10659,-10849,-11039,-11228,-11417,-11605,-11793,-11980,-12167,-12353,
    -12539,-12725,-12910,-13094,-13279,-13462,-13645,-13828,-14010,-14191,-14372,-14553,-14732,-14912,-15090,-15269,
    -15446,-15623,-15800,-15976,-16151,-16325,-16499,-16673,-16846,-17018,-17189,-17360,-17530,-17700,-17869,-18037,
    -18204,-18371,-18537,-18703,-18868,-19032,-19195,-19357,-19519,-19680,-19841,-20000,-20159,-20317,-20475,-20631,
    -20787,-20942,-21096,-21250,-21403,-21554,-21705,-21856,-22005,-22154,-22301,-22448,-22594,-22739,-22884,-23027,
    -23170,-23311,-23452,-23592,-23731,-23870,-24007,-24143,-24279,-24413,-24547,-24680,-24811,-24942,-25072,-25201,
    -25329,-25456,-25582,-25708,-25832,-25955,-26077,-26198,-26319,-26438,-26556,-26674,-26790,-26905,-27019,-27133,
    -27245,-27356,-27466,-27575,-27683,-27790,-27896,-28001,-28105,-28208,-28310,-28411,-28510,-28609,-28706,-28803,
    -28898,-28992,-29085,-29177,-29268,-29358,-29447,-29534,-29621,-29706,-29791,-29874,-29956,-30037,-30117,-30195,
    -30273,-30349,-30424,-30498,-30571,-30643,-30714,-30783,-30852,-30919,-30985,-31050,-31113,-31176,-31237,-31297,
    -31356,-31414,-31470,-31526,-31580,-31633,-31685,-31736,-31785,-31833,-31880,-31926,-31971,-32014,-32057,-32098,
    -32137,-32176,-32213,-32250,-32285,-32318,-32351,-32382,-32412,-32441,-32469,-32495,-32521,-32545,-32567,-32589,
    -32609,-32628,-32646,-32663,-32678,-32692,-32705,-32717,-32728,-32737,-32745,-32752,-32757,-32761,-32765,-32766,
    -32767,-32766,-32765,-32761,-32757,-32752,-32745,-32737,-32728,-32717,-32705,-32692,-32678,-32663,-32646,-32628,
    -32609,-32589,-32567,-32545,-32521,-32495,-32469,-32441,-32412,-32382,-32351,-32318,-32285,-32250,-32213,-32176,
    -32137,-32098,-32057,-32014,-31971,-31926,-31880,-31833,-31785,-31736,-31685,-31633,-31580,-31526,-31470,-31414,
    -31356,-31297,-31237,-31176,-31113,-31050,-30985,-30919,-30852,-30783,-30714,-30643,-30571,-30498,-30424,-30349,
    -30273,-30195,-30117,-30037,-29956,-29874,-29791,-29706,-29621,-29534,-29447,-29358,-29268,-29177,-29085,-28992,
    -28898,-28803,-28706,-28609,-28510,-28411,-28310,-28208,-28105,-28001,-27896,-27790,-27683,-27575,-27466,-27356,
    -27245,-27133,-27019,-26905,-26790,-26674,-26556,-26438,-26319,-26198,-26077,-25955,-25832,-25708,-25582,-25456,
    -25329,-25201,-25072,-24942,-24811,-24680,-24547,-24413,-24279,-24143,-24007,-23870,-23731,-23592,-23452,-23311,
    -23170,-23027,-22884,-22739,-22594,-22448,-22301,-22154,-22005,-21856,-21705,-21554,-21403,-21250,-21096,-20942,
    -20787,-20631,-20475,-20317,-20159,-20000,-19841,-19680,-19519,-19357,-19195,-19032,-18868,-18703,-18537,-18371,
    -18204,-18037,-17869,-17700,-17530,-17360,-17189,-17018,-16846,-16673,-16499,-16325,-16151,-15976,-15800,-15623,
    -15446,-15269,-15090,-14912,-14732,-14553,-14372,-14191,-14010,-13828,-13645,-13462,-13279,-13094,-12910,-12725,
    -12539,-12353,-12167,-11980,-11793,-11605,-11417,-11228,-11039,-10849,-10659,-10469,-10278,-10087,-9896,-9704,
    -9512,-9319,-9126,-8933,-8739,-8545,-8351,-8157,-7962,-7767,-7571,-7375,-7179,-6983,-6786,-6590,
    -6393,-6195,-5998,-5800,-5602,-5404,-5205,-5007,-4808,-4609,-4410,-4210,-4011,-3811,-3612,-3412,
    -3212,-3012,-2811,-2611,-2410,-2210,-2009,-1809,-1608,-1407,-1206,-1005,-804,-603,-402,-201};


const int16_t *wavetables[1]= { table_sine };
