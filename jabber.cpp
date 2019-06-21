#include <iostream>
#include <arpa/inet.h>
#include <err.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <tls.h>
#include <unistd.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>


int main(int argc, char *argv[])
{
  struct tls *ctx = NULL;
  struct sockaddr_in server;
  int writelen;
  int readlen;
  int sock;
  char buf[1024] =
      "<stream:stream to=\"tilda.center\" xmlns=\"jabber:client\" "
      "xmlns:stream=\"http://etherx.jabber.org/streams\" version=\"1.0\">";
  char starttls[512] = "<starttls xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\"/>";
  char preauth[512] =
      "<?xml version='1.0'?> <stream:stream xmlns=\"jabber:client\" "
      "version=\"1.0\" xmlns:stream=\"http://etherx.jabber.org/streams\" "
      "to=\"tilda.center\" xml:lang=\"en\" >";
  char conn[512] = "<stream:stream xmlns=\"jabber:client\" version=\"1.0\" "
                   "xmlns:stream=\"http://etherx.jabber.org/streams\" "
                   "to=\"tilda.center\" xml:lang=\"en\">";


  char username[1024];
  std::cout << "Username: " << std::flush;
  std::cin >> username;
  int userlen = strlen(username);
  char *password = getpass("Password: ");
  int passlen = strlen(password);
  int totallen = 3 + userlen + passlen;
  char buffer[totallen];
  sprintf(buffer+1, "%s %s", username, password);
  buffer[0] = 0;
  buffer[userlen+1] = 0;
  buffer[totallen-1] = 0;


  std::string b64text;
	BIO *bio, *b64;
	BUF_MEM *bufferPtr;

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
	BIO_write(bio, buffer, totallen);
	BIO_flush(bio);
	BIO_get_mem_ptr(bio, &bufferPtr);
	BIO_set_close(bio, BIO_NOCLOSE);
	BIO_free_all(bio);

	b64text = bufferPtr->data;
  b64text.resize(b64text.size()-4);

  std::cout << b64text << std::endl;
  std::string auth = "<auth xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" mechanism=\"PLAIN\">";
  auth += b64text;
  auth += "</auth>";


  /*
  ** initiate client context
  */

  if ((ctx = tls_client()) == NULL)
    err(1, "tls_client:");

  /*
  ** apply config to context
  */

  if (tls_configure(ctx, NULL) != 0)
    err(1, "tls_configure: %s", tls_error(ctx));

  sock = socket(AF_INET, SOCK_STREAM, 0);

  server.sin_port = htons(5222);
  server.sin_addr.s_addr = inet_addr("199.247.21.11");
  server.sin_family = AF_INET;

  if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
  {
    printf("erro socket\n");
    return (1);
  }

  write(sock, buf, strlen(buf));
  readlen = read(sock, buf, 1024);
  buf[readlen] = 0;
  printf("read: %s\n", buf);
  readlen = read(sock, buf, 1024);
  buf[readlen] = 0;
  printf("read: %s\n", buf);

  write(sock, starttls, strlen(starttls));
  readlen = read(sock, buf, 1024);
  buf[readlen] = 0;
  printf("read: %s\n", buf);

  if (tls_connect_socket(ctx, sock, "tilda.center") < 0)
  {
    printf("tls_connect error\n");
    printf("%s\n", tls_error(ctx));
    return (1);
  }

  /*
  ** STARTTLS
  */

  if ((writelen = tls_write(ctx, preauth, strlen(preauth))) < 0)
    err(1, "tls_write: %s", tls_error(ctx));

  printf("sent message: [%*.*s]\n", writelen, writelen, preauth);

  readlen = tls_read(ctx, buf, 1024);
  buf[readlen] = 0;
  printf("read: %s\n", buf);
  readlen = tls_read(ctx, buf, 1024);
  buf[readlen] = 0;
  printf("read: %s\n", buf);


  /*
  ** login to server
  */

  if ((writelen = tls_write(ctx, auth.data(), auth.size())) < 0)
    err(1, "tls_write: %s", tls_error(ctx));

  printf("sent message: [%*.*s]\n", writelen, writelen, auth.data());

  readlen = tls_read(ctx, buf, 1024);
  buf[readlen] = 0;
  printf("read: %s\n", buf);


  /*
  ** start stream
  */

  if ((writelen = tls_write(ctx, conn, strlen(conn))) < 0)
    err(1, "tls_write: %s", tls_error(ctx));

  printf("sent message: [%*.*s]\n", writelen, writelen, conn);

  readlen = 1;
  while (readlen > 0)
  {
    readlen = tls_read(ctx, buf, 1024);
    buf[readlen] = 0;
    printf("read: %s\n", buf);
  }

  /*
  ** clean up all
  */

  if (tls_close(ctx) != 0)
    err(1, "tls_close: %s", tls_error(ctx));
  tls_free(ctx);
  // tls_config_free(cfg);

  return (0);
}
