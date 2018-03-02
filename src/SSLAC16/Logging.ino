void printToSerial(const byte level, const char *format, ...)
{
  if(level < MASTER_LOG_LEVEL)
  {
    return;
  }
  
  va_list argv;
  va_start(argv, format);

  //TODO: add levels to the output  
  vsprintf(stream, format, argv);
  Serial.println(stream);

  va_end(argv);
}

