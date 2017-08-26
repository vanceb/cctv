// Find internal 1.1 reference voltage on AREF pin
void setup ()
{
  analogReference (INTERNAL);
  analogRead (A0);  // force voltage reference to be turned on
}
void loop () { }
