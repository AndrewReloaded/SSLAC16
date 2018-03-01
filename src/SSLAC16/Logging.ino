const byte LOG_LEVEL_INFO = 0;
const byte LOG_LEVEL_DEBUG = 1;
const byte LOG_LEVEL_WARN = 2;

void writeToSerial(const byte level, const char *format, ...)
{
  int count;
  va_list argv;
  va_start(argv, count);

  //TODO: add levels
  
  snprintf(stream, 1024, format, argv);
  Serial.print(stream);

  va_end(argv);
}

