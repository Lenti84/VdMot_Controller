/**HEADER*******************************************************************
  project : VdMot Controller

  author : SurfGargano, Lenti84

  Comments: derived from https://github.com/brunojoyal/PushoverESP32

  Version :

  Modifcations :


***************************************************************************
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE DEVELOPER OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License.
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Copyright (C) 2021 Lenti84  https://github.com/Lenti84/VdMot_Controller

*END************************************************************************/



#include <Arduino.h>
#include "Pushover.h"
#include <map>
#include "globals.h"
#include "VdmTask.h"
/*
const char *PUSHOVER_ROOT_CA = "-----BEGIN CERTIFICATE-----\n"
							   "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"
							   "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
							   "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"
							   "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"
							   "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
							   "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"
							   "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"
							   "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"
							   "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"
							   "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"
							   "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"
							   "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"
							   "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"
							   "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"
							   "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"
							   "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"
							   "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"
							   "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"
							   "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"
							   "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"
							   "-----END CERTIFICATE-----\n";
*/

//Pushover root certificate (valid from 11/10/2006 to 15/01/2038)
const char *PUSHOVER_ROOT_CA = "-----BEGIN CERTIFICATE-----\n"
                  "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"
                  "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
                  "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
                  "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n"
                  "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
                  "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n"
                  "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n"
                  "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n"
                  "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n"
                  "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n"
                  "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n"
                  "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n"
                  "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n"
                  "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n"
                  "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n"
                  "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n"
                  "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n"
                  "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n"
                  "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"
                  "MrY=\n"
                  "-----END CERTIFICATE-----\n";

const char *PUSHOVER_API_URL = "https://api.pushover.net/1/messages.json";

CPushover::CPushover()
{
	//client.setInsecure();
}

CPushover::CPushover(const char *token, const char *user) : _token(token), _user(user)
{
	//client.setInsecure();
}

int CPushover::send(CPushoverMessage newMessage)
{
	int connResult = 0;
	int responseCode = 0;
	bool beginResult = false;
	std::map<const char *, const char *> messageData;
	messageData["token"] = _token;
	messageData["user"] = _user;
	messageData["message"] = newMessage.message;
	messageData["title"] = newMessage.title;
	//messageData["url"] = newMessage.url;
	//messageData["url_title"] = newMessage.url_title;
	//messageData["html"] = newMessage.html?"1":"0";
	//messageData["priority"] = ((String)newMessage.priority).c_str();
	//messageData["sound"] = newMessage.sound;
	//messageData["timestamp"] = ((String)newMessage.timestamp).c_str();
	 //No attachment, so just a regular HTTPS POST request.
		
	//client.setInsecure();
	// Set the certificate
    client.setCACert(PUSHOVER_ROOT_CA);
	if (client.connected()) UART_DBG.println("Pushover already connected");
	uint8_t loop = 0;
	while (!client.connected()) {
		UART_DBG.println("Pushover try connect #"+String(loop+1));
		if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
        	syslog.log(LOG_DEBUG, "Pushover try connect #"+String(loop+1));
    	}
		if (connResult=client.connect("api.pushover.net", 443)) break;
		UART_DBG.println("Pushover not connected : "+String(connResult));
		if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
        	syslog.log(LOG_DEBUG, "Pushover not connected : "+String(connResult));
    	}
		VdmTask.yieldTask(1000);
		loop++;
		if ((loop>=5) && (!client.connected())) {
			UART_DBG.println("Pushover failed after 5 retries : port 443 not connected");
			if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
        		syslog.log(LOG_DEBUG, "Pushover failed after 5 retries : port 443 not connected");
    		}
			return -1;
		}
	}
	UART_DBG.println("Pushover port 443 connected");
	beginResult = myClient.begin(client,PUSHOVER_API_URL);
	myClient.addHeader("Content-Type", "application/json");
	myClient.addHeader("Connection","close");
	StaticJsonDocument<512> doc;
	std::map<const char *, const char *>::iterator it = messageData.begin();
	while(it!=messageData.end()){
		doc[it->first] = it->second;
		it++;
	}
	char postmessage[512];
	serializeJson(doc, postmessage);
	UART_DBG.println("Pushover output :"+String(beginResult)+" > "+String(postmessage));
	if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
        syslog.log(LOG_DEBUG, "Pushover output :"+String(beginResult)+" > "+String(postmessage));
    }
	responseCode = myClient.POST(postmessage);
	myClient.end();
	
	return responseCode;
}

void CPushover::setToken(const char *token)
{
	_token = token;
}

void CPushover::setUser(const char *user)
{
	_user = user;
}