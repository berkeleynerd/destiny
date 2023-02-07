#ifndef DST_MAPOFLONGLONGS
#define DST_MAPOFLONGLONGS

#include <unordered_map>
class MapOfLongLongs : public std::unordered_map<int64_t, int64_t>
{
	typedef std::unordered_map<int64_t, int64_t> parent_t;
public:
	
	//insert into the map.  If already present, change the old value.
	void insert(int64_t key, int64_t val) {
		parent_t::operator[](key) = val;
	}
	
	//special operator semantics, for retrieval, returns 0 if not found.
	int64_t operator [] (int64_t id) {
		parent_t::iterator i = find(id);
		if (i != end())
			return i->second;
		return 0;
	}

};


#endif  //DST_MAPOFLONGLONGS

