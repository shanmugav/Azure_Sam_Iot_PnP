

//these could change accordingly with the architecture

#ifndef __MEMS_COMMON_TYPES
#define __MEMS_COMMON_TYPES

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char u8_t;
typedef unsigned short int u16_t;
typedef unsigned int u32_t;
typedef int i32_t;
typedef short int i16_t;
typedef signed char i8_t;


typedef union
{
  i16_t i16bit[3];
  u8_t u8bit[6];
} Type3Axis16bit_U;

typedef union
{
  i16_t i16bit;
  u8_t u8bit[2];
} Type1Axis16bit_U;

typedef union
{
  i32_t i32bit;
  u8_t u8bit[4];
} Type1Axis32bit_U;

typedef enum
{
  MEMS_SUCCESS        =   0x01,
  MEMS_ERROR        =   0x00
} mems_status_t;

#ifdef __cplusplus
}
#endif

#endif /*__MEMS_COMMON_TYPES*/

