#include <string>
#include <sstream>
#include <ProxygenExtend/HttpGetMessageClient.h>
#include <ProxygenExtend/UtilsFunction.h>
#include <folly/portability/GFlags.h>
#include <chrono>
using namespace std;
using namespace ProxygenExtend;

DEFINE_string(c, "/etc/ssl/certs/ca-certificates.crt",
			  "Path to trusted CA file");  // default for Ubuntu
DEFINE_int32(timeout, 30000,
			 "connect timeout in milliseconds");
DEFINE_int32(time,600,"time delta to loop");
DEFINE_string(dns,"1zlife.cn","domain for server");

//ddns@1444389025196148.onaliyun.com
const string strAccessKeyID="LTAIQACWhKfp26pJ";
const string strAccessKeySecret="CTvOb85DPEfXTT4thThx5FLH7pmqRW";
void InitSSLSeed()
{
	EventBase *evb=EventBaseManager::get()->getEventBase();
	if(!evb)
	{
		return;
	}
	ProxygenExtend::HttpGetMessageClient *cli=ProxygenExtend::Utils::InitSSLSeed(evb,FLAGS_http_client_connect_timeout,FLAGS_ca_path);
	evb->loop();
	delete cli;
}
int main(int argc, char* argv[])
{
	gflags::ParseCommandLineFlags(&argc, &argv, true);
	google::InitGoogleLogging(argv[0]);
	google::InstallFailureSignalHandler();
	InitSSLSeed();

	EventBase *evb=EventBaseManager::get()->getEventBase();
	if(!evb)
	{
		return;
	}
	string strIP;
	while(1)
	{
		string strNewIP;
		{
			string strURL="https://ifconfig.me/ip";
			URL url(strURL);
			unique_ptr<HttpBaseClient> cli(new HttpBaseClient(evb,proxygen::HTTPMethod::GET,url));
			ProxygenExtend::HttpRequest(evb,cli.get(),FLAGS_timeout,FLAGS_timeout);
			evb->loop();
			unique_ptr<folly::IOBuf> responseBody=move(cli->getResponseBody());
			if(!ProxygenExtend::Utils::GetStrMessage(responseBody.get(),strNewIP))
			{
				continue;
			}
		}
		if(strIP != strNewIP)
		{
			string strURL="https://alidns.aliyuncs.com/?";

			vector<string> vStr;
			vStr.reserve(10);
			vStr.push_back("Format=xml");
			vStr.push_back("Version=2015-01-09");
			vStr.push_back("SignatureMethod=HMAC-SHA1");
			vStr.push_back("SignatureVersion=1.0");
			vStr.push_back("AccessKeyId="+strAccessKeyID);
			{
				chrono::system_clock::now();
				ostringstream oss;
				oss<<"Timestamp=";
				vStr.push_back(oss.str());
			}
			{

				const int cnLength=14;
				vector<unsigned char> vBuffer(cnLength);
				if(1 != RAND_bytes(&vBuffer[0],cnLength))
				{
					for(int i=0;i < cnLength;++i)
					{
						vBuffer[i]='0'+rand()%10;
					}
				}
				else
				{
					for(int i=0;i < cnLength;++i)
					{
						vBuffer[i]='0'+vBuffer[i]%10;
					}
				}
				vStr.push_back(string("SignatureNonce=")+string(vBuffer[0],cnLength));
			}
			sort(vStr.begin(),vStr.end());
			string tmpStr;
			for(auto const &str:vStr)
			{
				tmpStr+=str;
			}
			tmpStr=ProxygenExtend::Utils::CalculateSHA1(tmpStr);
			strURL
			URL url(strURL);
			HttpBaseClient *cli=new HttpBaseClient(evb,proxygen::HTTPMethod::GET,url);

			ProxygenExtend::HttpRequest(evb,cli,FLAGS_timeout,FLAGS_timeout);
			evb->loop();
			delete cli;
		}
		sleep(FLAGS_time);
	}
	return EXIT_SUCCESS;
}
