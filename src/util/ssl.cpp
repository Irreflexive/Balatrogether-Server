#include "util/ssl.hpp"
#include "util/logs.hpp"

#define TLS_KEY_LENGTH 2048

using namespace balatrogether;

// From https://wiki.openssl.org/index.php/Simple_TLS_Server
// and https://gist.github.com/nathan-osman/5041136

/* Generates a 2048-bit RSA key. */
EVP_PKEY *generate_key(bool debugMode)
{
  BIGNUM *bne = BN_new();
  if (!bne) {
    return NULL;
  }

  if (BN_set_word(bne, RSA_F4) != 1) {
    BN_free(bne);
    return NULL;
  }

  /* Allocate memory for the EVP_PKEY structure. */
  EVP_PKEY *pkey = EVP_PKEY_new();
  if (!pkey) {
    BN_free(bne);
    return NULL;
  }
  
  /* Generate the RSA key and assign it to pkey. */
  RSA* rsa = RSA_new();
  if (!rsa) {
    BN_free(bne);
    EVP_PKEY_free(pkey);
    return NULL;
  }

  if (RSA_generate_key_ex(rsa, TLS_KEY_LENGTH, bne, NULL) != 1) {
    BN_free(bne);
    EVP_PKEY_free(pkey);
    RSA_free(rsa);
    return NULL;
  }

  if (!EVP_PKEY_assign_RSA(pkey, rsa)) {
    BN_free(bne);
    EVP_PKEY_free(pkey);
    RSA_free(rsa);
    return NULL;
  }

  if (debugMode) {
    char* buffer;
    size_t len;
    FILE* fp = open_memstream(&buffer, &len);
    PEM_write_RSAPrivateKey(fp, rsa, NULL, 0, 0, NULL, NULL);
    fclose(fp);
    logger::debug << buffer;
  }
  
  /* The key has been generated, return it. */
  BN_free(bne);
  return pkey;
}

/* Generates a self-signed x509 certificate. */
X509 *generate_x509(EVP_PKEY *pkey)
{
  /* Allocate memory for the X509 structure. */
  X509 *x509 = X509_new();
  if (!x509) {
    return NULL;
  }
  
  /* Set the serial number. */
  ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
  
  /* This certificate is valid from now until exactly one year from now. */
  X509_gmtime_adj(X509_get_notBefore(x509), 0);
  X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);
  
  /* Set the public key for our certificate. */
  X509_set_pubkey(x509, pkey);
  
  /* We want to copy the subject name to the issuer name. */
  X509_NAME *name = X509_get_subject_name(x509);
  
  /* Set the country code and common name. */
  X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, (unsigned char *)"US", -1, -1, 0);
  X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, (unsigned char *)"balatrogether", -1, -1, 0);
  X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"localhost", -1, -1, 0);
  
  /* Now set the issuer name. */
  X509_set_issuer_name(x509, name);
  
  /* Actually sign the certificate with our key. */
  if (!X509_sign(x509, pkey, EVP_sha384())) {
    X509_free(x509);
    return NULL;
  }
  
  return x509;
}

SSL_CTX *ssl::create_context()
{
  const SSL_METHOD *method;
  SSL_CTX *ctx;

  method = TLS_server_method();

  ctx = SSL_CTX_new(method);
  if (!ctx) {
    perror("Unable to create SSL context");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  return ctx;
}

void ssl::configure_context(SSL_CTX *ctx, bool debugMode)
{
  /* Set the key and cert */
  EVP_PKEY *pkey = generate_key(debugMode);
  if (!pkey) exit(EXIT_FAILURE);
  X509 *x509 = generate_x509(pkey);
  if (!x509) {
    EVP_PKEY_free(pkey);
    exit(EXIT_FAILURE);
  }

  if (SSL_CTX_use_certificate(ctx, x509) <= 0) {
    ERR_print_errors_fp(stderr);
    EVP_PKEY_free(pkey);
    X509_free(x509);
    exit(EXIT_FAILURE);
  }

  if (SSL_CTX_use_PrivateKey(ctx, pkey) <= 0 ) {
    ERR_print_errors_fp(stderr);
    EVP_PKEY_free(pkey);
    X509_free(x509);
    exit(EXIT_FAILURE);
  }
  
  EVP_PKEY_free(pkey);
  X509_free(x509);
}