#include <iostream>
#include <string>
#include <time.h>
//#include "httpmodel.h"
using namespace std;
// int main(){
//     string date = "Wed, 10 Feb 2022 01:15:45 GMT";
//     string max_age = "30";
//     int year;
//     int month;
//     int day;
//     int hour;
//     int minute;
//     int second;

//     char str[10];
//     char wht[20];
//     sscanf(date.data(),"%s %d %s %d %d:%d:%d", wht, &day, str, &year, &hour, &minute, &second);
//     string mon_str = str;
//     if(mon_str == "Jan") {
//         month = 1;
//     } else if(mon_str == "Feb") {
//         month = 2;
//     } else if(mon_str == "Mar") {
//         month = 3;
//     } else if(mon_str == "Apr") {
//         month = 4;
//     } else if(mon_str == "May") {
//         month = 5;
//     } else if(mon_str == "Jun") {
//         month = 6;
//     } else if(mon_str == "Jul") {
//         month = 7;
//     } else if(mon_str == "Aug") {
//         month = 8;
//     } else if(mon_str == "Sep") {
//         month = 9;
//     } else if(mon_str == "Oct") {
//         month = 10;
//     } else if(mon_str == "Nov") {
//         month = 11;
//     } else if(mon_str == "Dec") {
//         month = 12;
//     }

//     cout << "year is "<<year <<" "<<month<<" "<<day<<" "<<hour<<" "<<minute<<" "<<second<<endl;
    
//     time_t now;
//     struct tm create_time;
//     double seconds;

//     time(&now);  /* get current time; same as: now = time(NULL)  */
//     create_time = *gmtime(&now);
//     cout<<"time now is"<<now<<endl;
//     create_time.tm_year = year - 1900; create_time.tm_hour = hour; create_time.tm_min = minute; create_time.tm_sec = second;
//     create_time.tm_mon = month-1;  create_time.tm_mday = day;
//     seconds = difftime(now, mktime(&create_time));
//     //printf ("", seconds); 
    
//     //"year"<<"month"<<"day"<<"hour"<<""
//     cout<<"time has passed"<<seconds<<" seconds"<<endl<<"if expired"<< (seconds - stoi(max_age) > 0)<<endl;
//     return 1;
// }


// int main() {
//     string headers = "name:oliver\r\nage:   88\r\nclass:568\r\n\r\n";
//     unordered_map<string,string> result = parseHeader(headers);
//     for(typename unordered_map<string,string>::iterator it = result.begin(); it != result.end(); ++it) {
//         cout<<it->first<<":"<<it->second<<endl;
//     }
//     return 1;
// }
string addtime(string date, string maxage){
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    char str[10];
    char wht[20];
    sscanf(date.data(),"%s %d %s %d %d:%d:%d", wht, &day, str, &year, &hour, &minute, &second);
    string mon_str = str;
    if(mon_str == "Jan") {
        month = 1;
    } else if(mon_str == "Feb") {
        month = 2;
    } else if(mon_str == "Mar") {
        month = 3;
    } else if(mon_str == "Apr") {
        month = 4;
    } else if(mon_str == "May") {
        month = 5;
    } else if(mon_str == "Jun") {
        month = 6;
    } else if(mon_str == "Jul") {
        month = 7;
    } else if(mon_str == "Aug") {
        month = 8;
    } else if(mon_str == "Sep") {
        month = 9;
    } else if(mon_str == "Oct") {
        month = 10;
    } else if(mon_str == "Nov") {
        month = 11;
    } else if(mon_str == "Dec") {
        month = 12;
    }

    
    time_t now;
    struct tm create_time;
    double seconds;

    time(&now);  /* get current time; same as: now = time(NULL)  */
    create_time = *gmtime(&now);
    create_time.tm_year = year - 1900; create_time.tm_hour = hour; create_time.tm_min = minute; create_time.tm_sec = second;
    create_time.tm_mon = month - 1;  create_time.tm_mday = day;
    now = timegm(&create_time);
    now += stoi(maxage);
    create_time = *gmtime(&now);
    return asctime(&create_time);
}
int main(){
    string date = "Wed, 21 Oct 2015 07:28:00 GMT" ;
    string seconds = "120";
    cout << date<<endl;
    cout << seconds << endl;
    cout << addtime(date,seconds) << endl;
    return 1;
}