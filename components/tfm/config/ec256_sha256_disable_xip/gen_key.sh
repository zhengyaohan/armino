openssl ecparam -name secp256r1 -genkey -out ec_256_privkey.pem
openssl ec -in ec_256_privkey.pem -inform PEM -out ec_256_pubkey.pem -pubout
