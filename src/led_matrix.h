#ifndef LED_MATRIX_H_
#define LED_MATRIX_H_

#define PIXEL_BIT(idx, val)  (val ? BIT(idx) : 0)
#define PIXEL_MASK(...)      (FOR_EACH_IDX(PIXEL_BIT, (|), __VA_ARGS__))

/* size of stack area used by each thread */
#define LED_MATRIX_STACKSIZE 1024

/* scheduling priority used by each thread */
#define LED_MATRIX_PRIORITY 7

int led_matrix_init(void);
void led_matrix_set_all(const uint8_t v);
void led_matrix_test(void);

#endif /* LED_MATRIX_H_ */