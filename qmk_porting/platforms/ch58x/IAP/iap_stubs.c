#include "../../platforms/ch58x/IAP/iap.h"

// Stubs for missing IAP wireless hooks
void iap_handle_new_wireless_chip() {
    // Stub implementation
}

bool iap_validate(uint32_t * address) {
    // Stub implementation
    return false;
}

void iap_handle_data(uint32_t start_address, uint8_t * data, uint32_t len) {
    // Stub implementation
}

// Stub for RSA key
const unsigned char rsa_pub_key[] = {
    // Empty key or dummy key
    0x00
};
const unsigned int rsa_pub_key_len = sizeof(rsa_pub_key);

