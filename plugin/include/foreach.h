// UnderC Development project
#ifndef __FOR_EACH_H
#define __FOR_EACH_H

// Enhanced FOREACH and FOREACH_AUTO macro by Hunter
// usage: vector<int> vec; vec.push_back(123);
//        FOREACH(int val, vec) { cout << val; }
//        FOREACH(int &val, vec) { val = 10; }
//        FOREACH(auto val, vec) { val = 20; }
//        FOREACH_AUTO(val, vec) { cout << val; }
//        FOREACH_AUTO(&val, vec) { val = 20; }
//        FOREACH_I(i, int val, vec) { vec[i] += val; }
//        FOREACH_I_AUTO(i, val, vec) { vec[i] += val; } 

template <class C>
    struct _ForEach2 
    {
        typename C::iterator m_it,m_end;
        _ForEach2(C& c) { m_it = c.begin(); m_end = c.end(); }
        auto deref() { return *m_it; }
        bool hasnext() { return m_it != m_end; }
        void next() { ++m_it; }
    };
 
template <>
    struct _ForEach2<char*>
    {
        char *m_it;
        char *m_end;
        
        _ForEach2(char *c) { m_it = c; m_end = c + strlen(c); }
        char &deref() { return *m_it; }
        bool hasnext() { return m_it < m_end; }
        void next() { ++m_it; }
    };
template <>
    struct _ForEach2<const char*>
    {
        const char *m_it;
        const char *m_end;
        
        _ForEach2(const char *c) { m_it = c; m_end = c + strlen(c); }
        const char &deref() { return *m_it; }
        bool hasnext() { return m_it < m_end; }
        void next() { ++m_it; }
    };
    
#define FOREACH(v,c) \
for(_ForEach2<typeof(c)> _fe(c);_fe.hasnext();) \
for(bool b=true;_fe.hasnext();_fe.next(), b=true) \
for(v = _fe.deref();b;b=false)

#define FOREACH_AUTO(v,c) FOREACH(auto v, c)

#define FOREACH_I(i,v,c) \
for(_ForEach2<typeof(c)> _fe(c);_fe.hasnext();) \
for(unsigned int i = 0;_fe.hasnext();++i) \
for(bool b=true;_fe.hasnext();++i, _fe.next(), b=true) \
for(v = _fe.deref();b;b=false)

#define FOREACH_I_AUTO(i,v,c) FOREACH_I(i, auto v, c)

/*
template <class C, class T>
   struct _ForEach {
     typename C::iterator m_it,m_end;
     T& m_var;
    _ForEach(C& c, T& t) : m_var(t)
     { m_it = c.begin(); m_end = c.end(); }

     bool get() { 
       bool res = m_it != m_end;
       if (res) m_var = *m_it;
       return res;
     }

     void next() { ++m_it; }
   };

#define FOR_EACH(v,c) for(_ForEach<typeof(c),typeof(v)> _fe(c,v); \
                          _fe.get();  _fe.next())
*/

#endif
