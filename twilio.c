/**
	@file
	twiliomax - a max object shell
	jeremy bernstein - jeremy@bootsquad.com	

	@ingroup	examples	
*/

#include "ext.h"							// standard Max include, always required
#include "ext_obex.h"						// required for new style Max object

#include <curl/curl.h>

#include "mongoose.h"

#include "clocaltunnel.h"

////////////////////////// object struct
typedef struct _twiliomax 
{
	t_object					ob;			// the object itself (must be first)
    
    void *m_outlet1;
    
    CURL *curl;
    
    char *twilio_account_sid;
    char *twilio_phone_number;
    
    struct mg_context *mongoose;
    struct clocaltunnel_client *clocaltunnel;
} t_twiliomax;

///////////////////////// function prototypes
//// standard set
void *twiliomax_new(t_symbol *s, long argc, t_atom *argv);
void twiliomax_free(t_twiliomax *x);
void twiliomax_assist(t_twiliomax *x, void *b, long m, long a, char *s);

void twiliomax_sendsms(t_twiliomax *x, t_symbol *s, long argc, t_atom *argv);
void twiliomax_receivesms(t_twiliomax *x, t_symbol *s, long argc, t_atom *argv);

//////////////////////// global class pointer variable
void *twiliomax_class;


int main(void)
{	
	t_class *c;
	
	c = class_new("twiliomax", (method)twiliomax_new, (method)twiliomax_free, (long)sizeof(t_twiliomax), 
				  0L /* leave NULL!! */, A_GIMME, 0);
	
    class_addmethod(c, (method)twiliomax_assist,			"assist",		A_CANT, 0);
    class_addmethod(c, (method)twiliomax_sendsms, "sendsms", A_GIMME, 0);
    class_addmethod(c, (method)twiliomax_receivesms, "receivesms", A_GIMME, 0);
	
	class_register(CLASS_BOX, c); /* CLASS_NOBOX */
	twiliomax_class = c;

	curl_global_init(CURL_GLOBAL_ALL);
    clocaltunnel_global_init();
	return 0;
}

void twiliomax_assist(t_twiliomax *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { // inlet
		sprintf(s, "I am inlet %ld", a);
	} 
	else {	// outlet
		sprintf(s, "I am outlet %ld", a); 			
	}
}

void twiliomax_free(t_twiliomax *x)
{
    curl_easy_cleanup(x->curl);
    
    if (x->mongoose) {
        mg_stop(x->mongoose);
    }
    
    if (x->clocaltunnel) {
        clocaltunnel_client_stop(x->clocaltunnel);
        clocaltunnel_client_free(x->clocaltunnel);
        
        clocaltunnel_global_cleanup();
    }
}


void *twiliomax_new(t_symbol *s, long argc, t_atom *argv)
{
	t_twiliomax *x = NULL;

	if ((x = (t_twiliomax *)object_alloc(twiliomax_class))) {
        if (argc != 3) {
            object_error((t_object *)x, "Please provide a twilio account SID, auth token, and outgoing phone number");
        } else if (argv[0].a_type != A_SYM || argv[1].a_type != A_SYM || argv[2].a_type != A_SYM) {
            object_error((t_object *)x, "All arguments should be strings");
        } else {
            x->m_outlet1 = outlet_new((t_object *)x, NULL);
            
            x->curl = curl_easy_init();
            object_post((t_object *)x, "libcurl initialized at %p", x->curl);
            
            x->twilio_account_sid = atom_getsym(&argv[0])->s_name;
            x->twilio_phone_number = atom_getsym(&argv[2])->s_name;
            
            char *twilio_account_auth_token = atom_getsym(&argv[1])->s_name;
            
            char userpass[strlen(x->twilio_account_sid)+strlen(twilio_account_auth_token)+1];
            
            sprintf(userpass, "%s:%s", x->twilio_account_sid, twilio_account_auth_token);
            
            curl_easy_setopt(x->curl, CURLOPT_USERPWD, userpass);
            
            x->mongoose = NULL;
            x->clocaltunnel = NULL;
        }
	}
	return (x);
}

static const char *twilio_response = "<?xml version='1.0' encoding='UTF-8' ?><Response></Response>";

static void *twiliomax_mongoose_callback(enum mg_event event,
                                         struct mg_connection *conn) {
    
    t_twiliomax *x = (t_twiliomax*)mg_get_user_data(conn);
    
    if (event == MG_NEW_REQUEST) {
        
        char post_data[1024],
                sms_from[sizeof(post_data)],
                sms_body[sizeof(post_data)];
        int post_data_len;
        
        // Read POST data
        post_data_len = mg_read(conn, post_data, sizeof(post_data));
        
        mg_get_var(post_data, post_data_len, "From", sms_from, sizeof(sms_from));
        mg_get_var(post_data, post_data_len, "Body", sms_body, sizeof(sms_body));
        
        object_post((t_object *)x, "Got sms From: %s Body: %s", sms_from, sms_body);
        
        t_atom sms_atoms[2];
        
        atom_setsym(&sms_atoms[0], gensym(sms_from));
        atom_setsym(&sms_atoms[1], gensym(sms_body));
        
        outlet_anything(x->m_outlet1, gensym("sms"), 2, sms_atoms);
        
        mg_printf(conn,
                  "HTTP/1.0 200 OK\r\n"
                  "Content-Type: application/xml\r\n\r\n"
                  "%s",
                  twilio_response);
        
        return "";
    } else {
        return NULL;
    }
}

void twiliomax_receivesms(t_twiliomax *x, t_symbol *s, long argc, t_atom *argv) {
    
    if (!x->mongoose) {
        const char *options[] = {"listening_ports", "8080", NULL};
        
        x->mongoose = mg_start(&twiliomax_mongoose_callback, x, options);
    }
    
    if (!x->clocaltunnel) {
        clocaltunnel_error err;
        
        x->clocaltunnel = clocaltunnel_client_alloc(&err);
        
        clocaltunnel_client_init(x->clocaltunnel, 8080);
        
        clocaltunnel_client_start(x->clocaltunnel, &err);
        
        while (clocaltunnel_client_get_state(x->clocaltunnel) < CLOCALTUNNEL_CLIENT_TUNNEL_OPENED) {
            //TODO wait on a semaphore with a timeout? give a callback? TBD
            usleep(50);
        }
        
        char external_url[50];
        
        strcpy(external_url, clocaltunnel_client_get_external_url(x->clocaltunnel));
        
        object_post((t_object *)x, "%s\n", external_url);
        
        //Update twilio configuration with new SMS url
        
    }
}


void twiliomax_sendsms(t_twiliomax *x, t_symbol *s, long argc, t_atom *argv) {

    
    if (argc != 2) {
        object_error((t_object *)x, "sendsms: Exactly two arguments required");
        return;
    }
    
    if (argv[0].a_type != A_SYM || argv[1].a_type != A_SYM) {
        object_error((t_object *)x, "sendsms: Invalid arg type. Exactly two string arguments required");
        return;
    }
    
    char *destination_number = atom_getsym(&argv[0])->s_name;
    char *message = atom_getsym(&argv[1])->s_name;
    
    object_post((t_object *)x, "Sending SMS to: %s Contents: %s", destination_number, message);
    
    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "From",
                 CURLFORM_COPYCONTENTS, x->twilio_phone_number,
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
    
    char *url_prefix = "https://api.twilio.com/2010-04-01/Accounts/";
    char *url_suffix = "/SMS/Messages.xml";
    
    char url[strlen(url_prefix)+strlen(x->twilio_account_sid)+strlen(url_suffix)+1];
    
    sprintf(url, "%s%s%s", url_prefix, x->twilio_account_sid, url_suffix);
    
    curl_easy_setopt(x->curl, CURLOPT_URL, url);
    curl_easy_setopt(x->curl, CURLOPT_HTTPPOST, formpost);
    
    CURLcode res = curl_easy_perform(x->curl);

    if(res == CURLE_OK) {
        object_post((t_object *)x, "CURL OK");
    } else {
        object_post((t_object *)x, "CURL FAILED");
    }
    
    curl_formfree(formpost);
}