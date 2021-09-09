void setup() {
  // put your setup code here, to run once:
  pinMode(8, OUTPUT);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(500);
  digitalWrite(8, HIGH);
  delay(500);
  digitalWrite(8, LOW);
  
}
