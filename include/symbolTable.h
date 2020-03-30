#pragma  once
#include <vector>
#include "symbolItem.h"



class symbolTable
{
    public:
        symbolTable() {}
        virtual ~symbolTable() {}
        void printTable();   //打印符号表
        
        /*填表操作*/
        bool ispushSymbolItem(std::string scope, std::string itemname);  //检查是否重复定义
        bool pushSymbolItem(std::string scope, std::string itemname, localMold mold, int value);  //int 类型localItem
        bool pushSymbolItem(std::string scope, std::string itemname, localMold mold, char value);   //char 类型 localItem
        bool pushSymbolItem(std::string scope, std::string arrayname, itemType it, int length);  //arrayItem
        bool pushSymbolItem(std::string scope, std::string funcName, funcReturnType frt);   //funcItem
        bool pushSymbolItem(std::string scope, std::string proname);  //proItem

        /*  上下文有关分析
         *    类型检查
         *    语义分析检查
         */
        bool searchTable();  //查表
        bool typeCheck(itemType stype, itemType dtype);   //类型检查


    private:
        std::vector<symbolItem> __symbolItem;    //符号表项
};
