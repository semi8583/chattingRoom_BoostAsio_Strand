#include <iomanip>
#include <fstream>
#include <iostream>

class ostreamFork           // Write same data to two ostreams
{
public:
    std::ostream& os1;
    std::ostream& os2;

    ostreamFork(std::ostream& os_one, std::ostream& os_two);

    template <class Data>
    friend ostreamFork& operator<<(ostreamFork& osf, Data d)
    {
        osf.os1 << d;
        osf.os2 << d;
        return osf;
    }
    
    // For manipulators: endl, flush
    friend ostreamFork& operator<<(ostreamFork& osf, std::ostream& (*f)(std::ostream&))
    {
        osf.os1 << f;
        osf.os2 << f;
        return osf;
    }

    // For setw() , ...
    template<class ManipData>
    friend ostreamFork& operator<<(ostreamFork& osf, std::ostream& (*f)(std::ostream&, ManipData))
    {
        osf.os1 << f;
        osf.os2 << f;
        return osf;
    }
};
