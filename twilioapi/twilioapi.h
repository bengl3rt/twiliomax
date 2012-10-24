//
//  twilioapi.h
//  twiliomax
//
//  Created by Ben Englert on 10/23/12.
//
//

#ifndef twiliomax_twilioapi_h
#define twiliomax_twilioapi_h

struct incoming_phone_number {
    char phone_number[100];
    char sid[100];
};

int get_incoming_phone_number(char *account_sid, CURL *the_curl, struct incoming_phone_number *phone_number);

int send_outgoing_sms(char *account_sid, CURL *the_curl, struct incoming_phone_number *phone_number, char *destination_number, char *message);

#endif
