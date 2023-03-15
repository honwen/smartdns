#include "client.h"
#include "include/utils.h"
#include "server.h"
#include "gtest/gtest.h"

TEST(Bind, tls)
{
	Defer
	{
		unlink("/tmp/smartdns-cert.pem");
		unlink("/tmp/smartdns-key.pem");
	};

	smartdns::Server server_wrap;
	smartdns::Server server;

	server.Start(R"""(bind [::]:61053
server-tls 127.0.0.1:60053 -no-check-certificate
log-num 0
log-console yes
log-level debug
cache-persist no)""");
	server_wrap.Start(R"""(bind-tls [::]:60053
address /example.com/1.2.3.4
log-num 0
log-console yes
log-level debug
cache-persist no)""");
	smartdns::Client client;
	ASSERT_TRUE(client.Query("example.com", 61053));
	ASSERT_EQ(client.GetAnswerNum(), 1);
	EXPECT_EQ(client.GetStatus(), "NOERROR");
	EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");
}

TEST(Bind, udp_tcp)
{
    smartdns::MockServer server_upstream;
	smartdns::MockServer server_upstream2;
	smartdns::Server server;

	server_upstream.Start("udp://0.0.0.0:61053", [](struct smartdns::ServerRequestContext *request) {
        unsigned char addr[4] = {1, 2, 3, 4};
		dns_add_A(request->response_packet, DNS_RRS_AN, request->domain.c_str(), 611, addr);
		request->response_packet->head.rcode = DNS_RC_NOERROR;
		return true;
	});

	server.Start(R"""(
bind [::]:60053
bind-tcp [::]:60053
server 127.0.0.1:61053
log-num 0
log-console yes
log-level debug
cache-persist no)""");
	smartdns::Client client;
	ASSERT_TRUE(client.Query("a.com +tcp", 60053));
	std::cout << client.GetResult() << std::endl;
	ASSERT_EQ(client.GetAnswerNum(), 1);
	EXPECT_EQ(client.GetStatus(), "NOERROR");
    EXPECT_EQ(client.GetAnswer()[0].GetTTL(), 3);
	EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");

	ASSERT_TRUE(client.Query("a.com", 60053));
	std::cout << client.GetResult() << std::endl;
	ASSERT_EQ(client.GetAnswerNum(), 1);
	EXPECT_EQ(client.GetStatus(), "NOERROR");
    EXPECT_EQ(client.GetAnswer()[0].GetTTL(), 611);
	EXPECT_EQ(client.GetAnswer()[0].GetData(), "1.2.3.4");

}

