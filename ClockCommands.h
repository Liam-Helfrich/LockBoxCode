#ifndef CLOCK_COMMANDS_H
#define CLOCK_COMMANDS_H

static timestamp32bits stamp;

String dayAsString(const Time::Day day) { //This function is taken from the DS1302.h sample code (see link in LockBoxCode.ino)
  switch (day) {
    case Time::kSunday: return "Sunday";
    case Time::kMonday: return "Monday";
    case Time::kTuesday: return "Tuesday";
    case Time::kWednesday: return "Wednesday";
    case Time::kThursday: return "Thursday";
    case Time::kFriday: return "Friday";
    case Time::kSaturday: return "Saturday";
  }
  return "(unknown day)";
}

String spanAsString(TimeSpan& span){
  return String("")+span.days()+"d "+span.hours()+"h "+span.minutes()+"m "+span.seconds()+"s";
}

String timeAsString(Time& t){
  return String(t.mon)+"/"+t.date+"/"+(t.yr-2000)+" "+t.hr+":"+(t.min < 10 ? "0" : "")+t.min+":"+(t.sec < 10 ? "0" : "")+t.sec;
}

uint32_t time_to_timestamp(Time& t){
  return stamp.timestamp(t.yr-2000, t.mon, t.date, t.hr, t.min, t.sec);
}


#endif
