# ima_service

IMA_SERVICE INCLUDES 3 PARTS:
  1) ima_server
    Can handle multiple clients. For help on usage enter "help" after starting server.
  2) ima_client
    Receives commands from the server and redirects them to the dbus_deamon.
  3) dbus_deamon
    API for DBus, provides 3 functions: status, addFile, rmFile.

INSTALLATION:

  Before building the code, you need to install the necessary packages.
  
  sudo apt install ima-evm-utils
  
  
  Then evmctl man page:
  
  Generate private key in plain text format:

  openssl genrsa -out privkey_evm.pem 1024

  Generate encrypted private key:

  openssl genrsa -des3 -out privkey_evm.pem 1024

  Make encrypted private key from unencrypted:

  openssl rsa -in /etc/keys/privkey_evm.pem -out privkey_evm_enc.pem -des3

  Generate self-signed X509 public key certificate and private key for using kernel asymmetric keys support:

  openssl req -new -nodes -utf8 -sha1 -days 36500 -batch \
              -x509 -config x509_evm.genkey \
              -outform DER -out x509_evm.der -keyout privkey_evm.pem

  Configuration file x509_evm.genkey:
  
  [ req ]
  
  default_bits = 1024
  
  distinguished_name = req_distinguished_name
  
  prompt = no
  
  string_mask = utf8only
  
  x509_extensions = myexts
  

  [ req_distinguished_name ]
  
  O = Magrathea
  
  CN = Glacier signing key
  
  emailAddress = slartibartfast@magrathea.h2g2
  

  [ myexts ]
  
  basicConstraints=critical,CA:FALSE
  
  keyUsage=digitalSignature
  
  subjectKeyIdentifier=hash
  
  authorityKeyIdentifier=keyid
  

  Generate public key for using RSA key format:

  openssl rsa -pubout -in privkey_evm.pem -out pubkey_evm.pem

  Copy keys to /etc/keys:
  
     cp pubkey_evm.pem /etc/keys
     scp pubkey_evm.pem target:/etc/keys
  or
  
     cp x509_evm.pem /etc/keys
     scp x509_evm.pem target:/etc/keys
     
  Then build the code:
  
  make
  make clean
  
  RUNNING:
  
  ./ima_server
  
  ./dbus_deamon
  
  ./ima_client
  
