
## Installing PortAudio
To add port audio run the script "install.sh"

## Building and running listener
To compile this project use make with key listen: **make listen**
To compile with debug option use key debug: **make debug**

#Thera are several keys:#

    -L "filename" - file for storing loggs(default:   "log_file.log")
    
    -i <integer> log level 1 -> log all information 5 -> only critical (default: 2)
    
    -c  - log to console  (default)
    
    -f  - log to file
    
    -b  - log to console and file
    
    -B <integer>   - packet size in bytes (default 1024)
    
    -a "ip address" - server address (default "127.0.0.1")
    
    -p  integer     - port address (default: 10043)
    
    -v  float   - sensivity of microphone (default: 0.003)
