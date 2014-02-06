#ifndef DST_MAPOFLONGLONGS
#define DST_MAPOFLONGLONGS

#include <hash_map>
class MapOfLongLongs : public stdext::hash_map<__int64, __int64>
{
	typedef stdext::hash_map<__int64, __int64> parent_t;
public:
	
	//insert into the map.  If already present, change the old value.
	void insert(__int64 key, __int64 val) {
		parent_t::operator[](key) = val;
	}
	
	//special operator semantics, for retrieval, returns 0 if not found.
	__int64 operator [] (__int64 id) {
		parent_t::iterator i = find(id);
		if (i != end())
			return i->second;
		return 0;
	}

};


#endif  //DST_MAPOFLONGLONGS

