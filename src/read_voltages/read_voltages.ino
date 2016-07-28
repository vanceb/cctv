#define BATTERY_PIN A7
#define SOLAR_PIN A2

int battery = 0;
int solar = 0;
const long InternalReferenceVoltage = 1075;  // Adjust this value to your board's specific internal BG voltage

int getBandgap ()
  {
  // REFS0 : Selects AVcc external reference
  // MUX3 MUX2 MUX1 : Selects 1.1V (VBG)
   ADMUX = bit (REFS0) | bit (MUX3) | bit (MUX2) | bit (MUX1);
   ADCSRA |= bit( ADSC );  // start conversion
   while (ADCSRA & bit (ADSC))
     { }  // wait for conversion to complete
   int results = (((InternalReferenceVoltage * 1024) / ADC) + 5) / 10;
   return results;
  } // end of getBandgap

void update_pwr() {
    // Enable ADC
    //ADCSRA = original_ADCSRA;
    // Wait for it to settle
    //delay(100);

    //battery = getBandgap();
    battery = analogRead(BATTERY_PIN);
    solar = analogRead(SOLAR_PIN);

    // disable ADC
     //ADCSRA = 0;
}

void setup() {
  // put your setup code here, to run once:
  pinMode(SOLAR_PIN, INPUT);
  pinMode(BATTERY_PIN, INPUT); 
  Serial.begin(9600); 
}

void loop() {
  // put your main code here, to run repeatedly:
  update_pwr();
  Serial.print("Battery: ");
  Serial.println(battery);
  Serial.print("Solar: ");
  Serial.println(solar);
  delay(1000);
}
