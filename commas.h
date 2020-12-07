#ifndef __COMMAS_H__
#define __COMMAS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The maximum unsigned 64-bit integer (18,446,744,073,709,551,615)
 * string contains (20) decimal digits, (6) commas, and a null char.
 */
#define MAXIMUM_UINT64_STRING_LENGTH (27)

extern int UInt64ToDigits(uint8_t *digits, uint64_t number);
extern uint32_t UInt64ToString(unsigned char *buff, uint64_t number);

#ifdef __cplusplus
}
#endif


#endif // !__COMMAS_H__
