#pragma  once
#include "lex.h"

/*
   @@分程序结构 符号表项设计
   子类继承方式   root根符号表对多类型符号表用vector管理
   @根符号表(父类)
   @函数符号表
   @过程符号表 (if-while作用域)
   @局部变量符号表
   @数组符号表 

*/
/*
   @@符号表管理方式
   localItem *li = new localItem(st_localType, "", "");  子类指针
   symbolItem *si;   父类指针
   li = static_cast<localItem *>(si);
   std::cout << li->getInteger() << std::endl;
   用父类的链表直接同意管理
   在访问子类元素的时候  将父类指针进行强制类型准换 即可
*/
class SymbolItem
{
    public:
        SymbolItem(symbolType st, std::string scope, std::string name);
        SymbolItem *next;   //指向下一个链表元素
        SymbolItem *prev;   //当前作用域失败，向上索引   前驱指针

        /*get*/
        inline symbolType getSt() {return _st;}
        inline int getCurrentLevel() { return _current_level;}
        inline std::string getname() { return _name;}
        inline std::string getscope() { return _scope;}
        inline int getIndex() { return _index;}

        /*set*/
        inline void setlevel(int level) { _current_level = level;}

    private:
        symbolType _st;   //当前符号表类型
        std::string _name;   //常量，变量，函数，数组    
        std::string _scope;   //当前符号表所属的函数作用域
        int _current_level;   //符号表作用域层次
        int _index;   //链表索引  (模拟数组访问索引)
};


//常量，变量，参数
class LocalItem : public SymbolItem {

    public:
        LocalItem(symbolType st, std::string scope, std::string itemname)
            :SymbolItem(st, scope, itemname) {}

        /*get*/
        inline localMold getLm() { return _lm;}
        inline itemType getIt() {return _it;}
        inline int getInteger() {return _iValue;}
        inline char getCharacter() {return _cValue;}

        /*set*/
        inline void setLm(localMold lm) { _lm = lm;}
        inline void setIt(itemType it) { _it = it;}
        inline void setInteget(int iValue) { _iValue = iValue;}
        inline void setCharacter(char cValue) { _cValue = cValue;}

    private:
        localMold _lm;   //常量&变量
        itemType _it;  //具体数据类型
        int _iValue;   //整形值
        char _cValue;   //字符值
};


//数组-一维
class ArrayItem : public SymbolItem
{
    public:
        ArrayItem (symbolType st, std::string scope, std::string itemname)
            :SymbolItem(st, scope, itemname) {}

        /*get*/
        inline int getLength() { return _length;}
        inline itemType getType() { return _it;}

        /*set*/
        inline void setLength(int length) { _length = length;}
        inline void setIt(itemType it) { _it = it;}

    private:
        itemType _it;   //具体数据类型
        int _length;   //数组长度
};


//函数
class FuncItem : public SymbolItem
{
    public:
        /*如果父类只存在有参数的构造函数，那子类必须显示调用其构造函数  借此完成父类的成员初始化*/
        FuncItem (symbolType st, std::string scope, std::string itemname) 
            : SymbolItem(st, scope, itemname){}

        /*get*/
        inline funcReturnType getReturnType() { return _frt;}

        /*set*/
        inline void setReturnType(funcReturnType frt) { _frt = frt;}

    private:
        funcReturnType _frt;    //函数返回类型

};


//过程
class ProcItem : public SymbolItem 
{
    public:
        ProcItem(symbolType st, std::string scope, std::string itemname)
            :SymbolItem(st, scope, itemname) {}

    private:

};

