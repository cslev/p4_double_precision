#include <bm/bm_sim/extern.h>

using namespace std;


template <typename... Args>
using ActionPrimitive = bm::ActionPrimitive<Args...>;

using bm::Data;
using bm::Header;
using bm::PHV;
using bm::ExternType;

class ExternIncrease : public ExternType
{
  public:
    BM_EXTERN_ATTRIBUTES
    {
      BM_EXTERN_ATTRIBUTE_ADD(attribute_example);
    }
    //init variables
    void init() override
    {}
    void increase() //int operand)
    {
      cout << "+--------------------------------+" << endl;
      cout << "|Increase extern has been called!|" << endl;
      cout << "+--------------------------------+" << endl;
      // cout << "added argument is : " << operand << endl;
    }

    //default deconstructor
    virtual ~ExternIncrease() {}

  private:
    Data attribute_example;
};
BM_REGISTER_EXTERN(ExternIncrease);
BM_REGISTER_EXTERN_METHOD(ExternIncrease, increase);

 // end declaration
 int import_extern_increase()
 {
   return 0;
 }
