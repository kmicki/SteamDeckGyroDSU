void Log(std::string message,LogLevel type = LogLevelDefault) { logbase::Log(cLogPrefix,message,type); }
logbase::LogF LogF(LogLevel type = LogLevelDefault) { return logbase::LogF(cLogPrefix,type); }
std::runtime_error runtime_error(const std::string& what_arg) { return runtime_error(cLogPrefix + what_arg); }