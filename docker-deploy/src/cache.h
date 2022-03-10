#include "httpmodel.h"
using namespace std;
class Cache{
    unordered_map<string,Http_Response> cache_map;
    size_t cache_size;
public:
    Cache(size_t size):cache_size(size);

    bool is_in_cache(string startline){
        if(cache_map.find(startline) != cache_map.end()){
            return true;
        }else{
            return false;
        }
    }

    Http_Response get_cache_response(string startline){
        return cache_map[startline];
    }

    void put_cache_responce(string startline,Http_Response response){
        cache_map[startline] = response;
        return;
    }
};
