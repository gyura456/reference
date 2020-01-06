#!/usr/bin/awk -f

BEGIN {
    ch0_maxtime = 0;
    ch0_max = 0;
    ch1_maxtime = 0;
    ch1_max = 0;
    ch2_maxtime =0;
    ch2_max = 0;
    reg_time0=0;
    reg_time1=0;
    reg_time2=0;
}
 $2 > ch0_max{
    ch0_max = $2;
    ch0_maxtime = $1;
 }
{
 if($2 >= 114 && $2 <= 118){
    if (reg_time0 == 0){
	reg_time0 = $1;
	}
    }
 else
     reg_time0 = 0;
}

 $3 > ch1_max{
    ch1_max = $3;
    ch1_maxtime = $1;
 }
{
 if($3 >= 114 && $3 <= 118){
    if (reg_time1 == 0){
	reg_time1 = $1;
	}
    }
 else
     reg_time1 = 0;
}

 $4 > ch2_max{
    ch2_max = $4;
    ch2_maxtime = $1;
 }
{
 if($4 >= 114 && $4 <= 118){
    if (reg_time2 == 0){
	reg_time2 = $1;
	}
    }
 else
     reg_time2 = 0;
}
END{
    printf "File: %s\n", ARGV[1];
    printf "Total time: %d s\n", $1;
    printf "Time of regulation CH0: %d s\n", reg_time0;
    printf "Time of regulation CH1: %d s\n", reg_time1;
    printf "Time of regulation CH2: %d s\n", reg_time2;
    printf "CH0 maximun temperature: %3.3f °C at %d s\n", ch0_max, ch0_maxtime;
    printf "CH1 maximun temperature: %3.3f °C at %d s\n", ch1_max, ch1_maxtime;
    printf "CH2 maximun temperature: %3.3f °C at %d s\n", ch2_max, ch2_maxtime;
}
