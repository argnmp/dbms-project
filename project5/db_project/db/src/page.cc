#include "page.h"
int64_t slot_t::get_key(){
    int64_t key;
    memcpy(&key, data, sizeof(key)); 
    return key;
}
uint16_t slot_t::get_size(){
    uint16_t size;
    memcpy(&size, data + 8, sizeof(size));
    return size;
}
uint16_t slot_t::get_offset(){
    uint16_t offset;
    memcpy(&offset, data + 10, sizeof(offset));
    return offset;
}
int slot_t::get_trx(){
    int trx;
    memcpy(&trx, data + 12, sizeof(trx));
    return trx;
}
void slot_t::set_key(int64_t key){
    memcpy(data, &key, sizeof(key)); 
}
void slot_t::set_size(uint16_t size){
    memcpy(data+8, &size, sizeof(size));
}
void slot_t::set_offset(uint16_t offset){
    memcpy(data+10, &offset, sizeof(offset));
}
void slot_t::set_trx(int trx){
    memcpy(data+12, &trx, sizeof(trx));
}

int64_t kpn_t::get_key(){
    int64_t key;
    memcpy(&key, data, sizeof(key)); 
    return key;
}
uint64_t kpn_t::get_pagenum(){
    uint64_t pagenum;
    memcpy(&pagenum, data + 8, sizeof(pagenum)); 
    return pagenum;
}
void kpn_t::set_key(int64_t key){
    memcpy(data, &key, sizeof(key)); 
}
void kpn_t::set_pagenum(uint64_t pagenum){
    memcpy(data + 8, &pagenum, sizeof(pagenum)); 
}
