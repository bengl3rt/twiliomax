//
//  twilioapi.c
//  twiliomax
//
//  Created by Ben Englert on 10/23/12.
//
//



#include <stdio.h>
#include <curl/curl.h>

#include "ext.h"

#include "jsmn/jsmn.h"

#include "twilioapi.h"

char *url_prefix = "https://api.twilio.com/2010-04-01/Accounts/";

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t twilioWriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    if (mem->memory == NULL) {
        return realsize;
    }
    
	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        exit(EXIT_FAILURE);
	}
    
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
    
	return realsize;
}

int get_incoming_phone_number(char *account_sid, CURL *the_curl, struct incoming_phone_number *phone_number) {
    char *url_suffix = "/IncomingPhoneNumbers.json";
    
    char url[strlen(url_prefix)+strlen(account_sid)+strlen(url_suffix)+1];
    
    sprintf(url, "%s%s%s", url_prefix, account_sid, url_suffix);
    
    curl_easy_setopt(the_curl, CURLOPT_URL, url);
    
    struct MemoryStruct chunk;
    
	chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */
    
	/* send all data to this function  */
	curl_easy_setopt(the_curl, CURLOPT_WRITEFUNCTION, twilioWriteMemoryCallback);
    
	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(the_curl, CURLOPT_WRITEDATA, (void *)&chunk);
    
    CURLcode res = curl_easy_perform(the_curl);
    
    if (res != CURLE_OK) {
        
        free(chunk.memory);
        return -1;
    }
    
    jsmn_parser p;
	jsmntok_t tok[100];
	
    memset(tok, -1, 100);
    
	jsmn_init(&p);
    
	if (jsmn_parse(&p, chunk.memory, tok, 100)) {
        post("JSON PARSE ERROR");
        free(chunk.memory);
        return -2;
    }
    
    int n;
    
    char current[10000];
    
    for (n = 0; n < 100; n++) {
        switch (tok[n].type) {
            case JSMN_PRIMITIVE:
                //post("Primitive");
                break;
            case JSMN_OBJECT:
                //post("Object");
                break;
            case JSMN_ARRAY:
                //post("Array");
                break;
            case JSMN_STRING:
                memcpy(current, chunk.memory+tok[n].start, tok[n].end - tok[n].start);
                current[tok[n].end - tok[n].start] = '\0';
                
                int m = n + 1;
                
                if (0 == strncmp(current, "sid", 3)) {
//                    post("Found SID key");
                    memcpy(phone_number->sid, chunk.memory+tok[m].start, tok[m].end - tok[m].start);
                    phone_number->sid[tok[m].end - tok[m].start] = '\0';
                    
                    
//                    post(phone_number->sid);
               }
                
                
                if (0 == strncmp(current, "phone_number", 3)) {
//                    post("Found phone number key");
                    memcpy(phone_number->phone_number, chunk.memory+tok[m].start, tok[m].end - tok[m].start);
                    phone_number->phone_number[tok[m].end - tok[m].start] = '\0';
                    
//                    post(phone_number->phone_number);
                    
                }
                
                break;
            default:
                //post("Other, bailing... %d", tok[n].type);
                goto bail;
        }
    }
    
bail:
    
    free(chunk.memory);
    return 0;
}


int send_outgoing_sms(char *account_sid, CURL *the_curl, struct incoming_phone_number *phone_number, char *destination_number, char *message) {
    struct MemoryStruct chunk;
    chunk.memory = NULL;
    
    curl_easy_setopt(the_curl, CURLOPT_WRITEFUNCTION, twilioWriteMemoryCallback);
	curl_easy_setopt(the_curl, CURLOPT_WRITEDATA, (void *)&chunk);

    post("Sending SMS to: %s Contents: %s From: %s", destination_number, message, phone_number->phone_number);

    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "From",
                 CURLFORM_COPYCONTENTS, phone_number->phone_number,
                 CURLFORM_END);
    
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "To",
                 CURLFORM_COPYCONTENTS, destination_number,
                 CURLFORM_END);
    
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "Body",
                 CURLFORM_COPYCONTENTS, message,
                 CURLFORM_END);
    
    char *url_suffix = "/SMS/Messages.xml";
    
    char url[strlen(url_prefix)+strlen(account_sid)+strlen(url_suffix)+1];
    
    sprintf(url, "%s%s%s", url_prefix, account_sid, url_suffix);
    
    curl_easy_setopt(the_curl, CURLOPT_URL, url);
    curl_easy_setopt(the_curl, CURLOPT_HTTPPOST, formpost);
    
    CURLcode res = curl_easy_perform(the_curl);
    
    if(res != CURLE_OK) {
        post("CURL FAILED");
        return -1;
    }
    
    curl_formfree(formpost);
    
    return 0;
}


int set_sms_url(char *account_sid, CURL *the_curl, struct incoming_phone_number *phone_number, char *sms_url) {
    struct MemoryStruct chunk;
    chunk.memory = NULL;
    
    curl_easy_setopt(the_curl, CURLOPT_WRITEFUNCTION, twilioWriteMemoryCallback);
	curl_easy_setopt(the_curl, CURLOPT_WRITEDATA, (void *)&chunk);
    
    char fqdn[50];
    sprintf(fqdn, "http://%s", sms_url);
    
    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "SmsUrl",
                 CURLFORM_COPYCONTENTS, fqdn,
                 CURLFORM_END);
    
    char url_suffix[100];
    sprintf(url_suffix, "/IncomingPhoneNumbers/%s.json", phone_number->sid);
    
    char url[strlen(url_prefix)+strlen(account_sid)+strlen(url_suffix)+1];
    
    sprintf(url, "%s%s%s", url_prefix, account_sid, url_suffix);
    
    curl_easy_setopt(the_curl, CURLOPT_URL, url);
    curl_easy_setopt(the_curl, CURLOPT_HTTPPOST, formpost);
    
    CURLcode res = curl_easy_perform(the_curl);
    
    if(res != CURLE_OK) {
        post("CURL FAILED");
        return -1;
    }
    
    curl_formfree(formpost);
    
    return 0;
}