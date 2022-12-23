#ifndef LED_MATRIX_H_
#define LED_MATRIX_H_

#define PIXEL_BIT(idx, val)  (val ? BIT(idx) : 0)
#define PIXEL_MASK(...)      (FOR_EACH_IDX(PIXEL_BIT, (|), __VA_ARGS__))

void led_matrix_test(void);

#endif /* LED_MATRIX_H_ */