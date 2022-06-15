#openssl ecparam -name secp256r1 -genkey -out ec_256_privkey.pem
#openssl ec -in ec_256_privkey.pem -inform PEM -out ec_256_pubkey.pem -pubout

openssl genrsa -out rsa_2048_privkey.pem 2048
openssl rsa -in rsa_2048_privkey.pem -inform PEM -out rsa_2048_pubkey.pem -pubout
