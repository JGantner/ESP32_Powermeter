#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <OakOLED.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ADS1015.h>

OakOLED oled;
Adafruit_ADS1115 ads;
unsigned long last_time = 0;
unsigned long current_time = 0;



class measurement
{
public:
    int16_t _adc_val = 0;
    unsigned long _time = 0;

    measurement(int16_t adc_val, unsigned long time): _adc_val(adc_val), _time(time) { }
    measurement(): measurement(0,0) { }
};

class power_meter
{
public:

    void start_measuring(unsigned long start_time);
    void stop_measuring(unsigned long stop_time);
    void add_measurement(int16_t adc_val, unsigned long measurement_time);

    unsigned long get_measurement_duration();
    int16_t get_rms_value();

private:
    long _sample_counter = 0;
    measurement _measurements[1000];
    unsigned long _start_time = 0;
    unsigned long _stop_time = 0;
};

void power_meter::start_measuring(unsigned long start_time)
{
    _start_time = start_time;
    _stop_time = 0;
    _sample_counter = -1;
}

void power_meter::stop_measuring(unsigned long stop_time)
{
    _stop_time = stop_time;
}

void power_meter::add_measurement(int16_t adc_val, unsigned long measurement_time)
{
    if(_sample_counter >= 1000) return;

    _sample_counter++;
    _measurements[_sample_counter] = measurement(adc_val, measurement_time);

}

unsigned long power_meter::get_measurement_duration()
{
    return _stop_time - _start_time;
}

int16_t power_meter::get_rms_value()
{
    if(_sample_counter < 0 ) return 0;

    double squared_value = 0;
    double sum_squared_values = 0;
    double mean_squared_values = 0;

    measurement *current_measurement = nullptr;


    for(size_t current_sample = 0; current_sample <= _sample_counter; current_sample++)
    {
        current_measurement = &_measurements[current_sample];
        squared_value = (double)current_measurement->_adc_val * current_measurement->_adc_val;

        sum_squared_values += squared_value;
    }

    mean_squared_values = (double)sum_squared_values / _sample_counter;

    return (int16_t) sqrt(mean_squared_values);
}

power_meter pm;

void setup() {
    // put your setup code here, to run once:
    oled.begin();
    oled.setTextColor(1);
    oled.setTextSize(1);

    ads.setGain(GAIN_FOUR);
    ads.begin();
}

void loop() {
    // put your main code here, to run repeatedly:

    oled.clearDisplay();
    oled.println("measuring ...");
    oled.display();

    int16_t adc0 = 0;
    float measured_voltage;
    const float adc_to_mV_factor = 0.03125F;
    const float mV_to_mAmp_factor = 1000 /* mv*/ / 30.0 /*A*/ ;
    unsigned long loop_time = 0;
    pm.start_measuring(millis());

    for(int i=0; i<200; i++)
    {
        adc0 = ads.readADC_Differential_0_1();
        pm.add_measurement(adc0, millis());
    }

    pm.stop_measuring(millis());

    oled.clearDisplay();
    oled.setCursor(0,0);
    measured_voltage = (float)pm.get_rms_value() * adc_to_mV_factor;
    oled.println("Voltage [mV]: " + String(measured_voltage));

    oled.println("loop [ms]: " + String(loop_time));
    oled.println("measure [ms]: " + String(pm.get_measurement_duration()));
    oled.println("current [mA]: " + String(measured_voltage * mV_to_mAmp_factor));
    last_time = current_time;


    oled.display();


    delay(5000);
}