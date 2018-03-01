void printToSerial(const byte level, const char *format, ...)
{
  va_list argv;
  va_start(argv, format);

  //TODO: add levels
  
  vsprintf(stream, format, argv);
  Serial.println(stream);

  va_end(argv);
}

