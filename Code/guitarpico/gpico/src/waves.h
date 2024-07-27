/* waves.h

*/

/*
   Copyright (c) 2024 Daniel Marks

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _WAVES_H
#define _WAVES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define WAVETABLES_NUMBER 1
#define WAVETABLES_LENGTH 1024

extern const int16_t *wavetables[WAVETABLES_NUMBER];

extern const int16_t table_sine[];
 
#ifdef __cplusplus
}
#endif

#endif // _WAVES_H
