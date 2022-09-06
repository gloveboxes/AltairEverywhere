#ifdef __cplusplus
extern "C"{
#endif 
//
// Reads air pressure from LPS25H
//
int get_pressure();

//
// Reads air pressure from LPS25H
//
float get_temperature_from_lps25h();

//
// Reads the temperature from HTS221
//
float get_temperature();

//
// Reads the humidity from HTS221
//
float get_humidity();

//
// Initializes i2c
//
int pi_sense_hat_sensors_init(int i2c_num);

//
// closes i2c
//
void pi_sense_hat_sensors_close(void);

#ifdef __cplusplus
}
#endif