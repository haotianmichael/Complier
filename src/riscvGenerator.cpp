#include "../include/riscvGenerator.h"
#include "../include/intermediateGenerator.h"
#include "../include/symbolTable.h"
#include "../include/utils.h"
#include <fstream>


extern IntermediateGenerator itgenerator;   //四元式产生表

void riscvGenerator::printAsmCode(Parser &p) {

    p.printParser();  //打印符号表和四元式
    std::cout << "Starting To Print RiscvCodeFile..." << std::endl;
    std::ofstream out("riscvCodeFile.s");
    out << "    .file  " << "\"" << this->filename << "\"" << std::endl;
    out << "    .option nopic" << std::endl; 

    /*全局变量 .srodataa  变量 常量 数组*/
    out << "    .text" << std::endl;
    SymbolItem *head = p.getSymbolTable()->getHead();
    SymbolItem *tail = p.getSymbolTable()->getTail();
    int flag = false;
    while(head != tail) {
        if(!(head->getSt() == st_localType && head->getscope() == "Global")) { 
            break; 
        } 
        LocalItem *li = static_cast<LocalItem *>(head);
        if(li->getisArr()) {  //数组
            //名字  类型
            std::string name = li->getname();
            itemType it = li->getIt();
            int length = li->getLength();
            if(it == it_intType) {
                out << "    .comm   " << name << ","<< length*4 << ",4" << std::endl;
            }else if(it == it_charType) {
                out << "    .comm   " << name << "," << length << ",4" << std::endl;
            }

        }else {
            if(li->getLm() == lm_constant) {  //常量
                // 名字   数据类型  值
                std::string name = li->getname();
                itemType it = li->getIt();
                out << "    .globl  " << name  << std::endl;
                if(it == it_intType) {
                    int value = li->getInteger();
                    if(!flag) {
                        out << "    .section    .srodata,\"a\"" << std::endl;;
                        flag = true; 
                    }
                    out << "    .align  2" << std::endl; 
                    out << "    .type   " << name << ", @object" << std::endl;
                    out << "    .size   " << name << ", 4" << std::endl;
                    out << name << ":" << std::endl;
                    out << "    .word   " << value << std::endl;

                }else if(it == it_charType) {
                    int cvalue = li->getCharacter();
                    out << "    .type   " << name << ", @object" << std::endl;
                    out << "    .size   " << name << ", 1" << std::endl;
                    out << name << ":" << std::endl;
                    out << "    .byte   " << cvalue << std::endl;
                }

            }else if(li->getLm() == lm_variable) {  // 变量
                //名字  类型
                std::string name = li->getname();
                itemType it = li->getIt();
                if(it == it_intType) {
                    out << "    .comm   " << name << ",4,4" << std::endl;
                }else if(it == it_charType) {
                    out << "    .comm   " << name << ",1,1" << std::endl; 
                }
            }
        }
        head = head->next;
    } 

    /*.rodata*/
    if(!itgenerator.dataSet.empty()) {
        out << "    .section    .rodata" << std::endl;
    }
    std::map<std::string, std::string>::iterator iter;  //遍历.data
    for(iter = itgenerator.dataSet.begin(); iter != itgenerator.dataSet.end(); iter++) {
        out << "    .align  2" << std::endl; 
        out << iter->second <<  ":" << std::endl;
        out << "    .string \"" << iter->first << "\"" << std::endl;
    }

    /* 函数模型
     * 1. 有main函数  无自定义函数  无系统函数(指printf scanf)   ismain == true && isfunc == false
     * 2. 有main函数  无自定义函数  有系统函数(指printf scanf)   ismain == true && isfunc == true
     * 3. 有main函数  有自定义函数  有系统函数(指printf scanf)   ismain == false 
     * 
     */
    if(itgenerator.getIntermediateList()[0].getopcode() != FUNDEC) {  //四元式一定以函数开始
        panic("CodeGenError: Wrong Instruction!"); 
    }

    bool ismain = true, isfunc = false;
    if(itgenerator.getIntermediateList()[0].gettarget() == "main") {  
        for(unsigned int i = 0; i < itgenerator.getIntermediateList().size(); i ++) {
            FourYuanInstr tmp = itgenerator.getIntermediateList()[i];
            fourYuanOpcode fy = tmp.getopcode();
            if(fy == PrintId || fy == PrintInt || fy == PrintStr || fy == PrintChar || fy == ReadInt || fy == ReadChar) {
                isfunc = true;
            }
        }
    }else if(itgenerator.getIntermediateList()[0].gettarget() != "main") {
        ismain = false; 
    }

    if(ismain && !isfunc) {   //无系统函数  无自定义函数
        /*  只需要维护main函数栈
         *  变量作用域：
         *     1. main函数内部  压栈
         *     2. main函数外部(全局变量) 使用Label
         *  四元式类型：
         *      赋值语句   运算语句  控制语句
         * */
        int fp = 16;
        //计算栈帧
        SymbolItem *head = p.getSymbolTable()->getHead();
        SymbolItem *tail = p.getSymbolTable()->getTail();
        int tmp = 0;
        while(head != tail) {
            if(head->getSt() == st_localType && head->getscope() == "main") {   //寻找main函数中的局部变量
                tmp++; 
            } 
            head = head->next; 
        }
        if(head->getSt() == st_localType && head->getscope() == "main"){
            tmp++; 
        }

        if(tmp%4 != 0) {
            tmp = tmp/4+1; 
        }else {
            tmp = tmp / 4; 
        }
        fp += tmp*16; 
        out << "    .text" << std::endl;
        out << "    .align  1" << std::endl;
        out << "    .global main"  << std::endl;
        out << "    .type   main" << ", @function" << std::endl;
        out << "main:" << std::endl; 
        out << "    addi sp, sp, -" << fp << std::endl;
        out << "    sw s0, " << fp-4 <<"(sp)" << std::endl;
        out << "    addi s0, sp, "<< fp << std::endl;

        /*函数内部翻译*/
        nofun_asmCodeGen(out, p);

        out << "    li a5, 0" << std::endl;
        out << "    mv a0, a5" << std::endl;
        out << "    lw s0, "<< fp-4 << "(sp)" << std::endl;
        out << "    addi sp, sp, "<< fp << std::endl;
        out << "    jr ra" << std::endl;
        out << "    .size   main, .-main" << std::endl;
    }else if(ismain && isfunc){     //有系统函数  无自定义函数
        int fp = 16;
        //计算栈帧
        SymbolItem *head = p.getSymbolTable()->getHead();
        SymbolItem *tail = p.getSymbolTable()->getTail();
        int tmp = 0;
        while(head != tail) {
            if(head->getSt() == st_localType && head->getscope() == "main") {   //寻找main函数中的局部变量
                LocalItem *li = static_cast<LocalItem *>(head); 
                if(li->getisArr()) {
                    tmp += li->getLength(); 
                }else {
                    tmp++; 
                }
            } 
            head = head->next; 
        }
        if(head->getSt() == st_localType && head->getscope() == "main"){
            LocalItem *li = static_cast<LocalItem *>(head);
            if(li->getisArr()) {
                tmp += li->getLength(); 
            }else {
                tmp ++; 
            }
        }

        if(tmp%4 != 0) {
            tmp = tmp/4+1; 
        }else {
            tmp = tmp / 4; 
        }
        fp += tmp*16; 
        //为系统函数分配标签
        if(itgenerator.dataSet.empty()) {
            out << "    .section    .rodata" << std::endl;
        }
        out << "    .align 2" << std::endl; 
        out << ".PL0:" << std::endl;
        out << "    .string \"%d\"" << std::endl;    
        out << "    .text" << std::endl;

        out << "    .align  1" << std::endl;
        out << "    .global main"  << std::endl;
        out << "    .type   main" << ", @function" << std::endl;
        out << "main:" << std::endl; 
        out << "    addi sp, sp, -" << fp << std::endl;
        out << "    sw ra, " << fp-4<< "(sp)" << std::endl;
        out << "    sw s0, "<< fp-8<< "(sp)" << std::endl;
        out << "    addi s0, sp, " << fp << std::endl;

        /*函数内部翻译*/
        nofun_asmCodeGen(out, p);


        out << "    li a5, 0" << std::endl;
        out << "    mv a0, a5" << std::endl;
        out << "    lw ra, " << fp-4<< "(sp)" << std::endl;
        out << "    lw s0, " << fp-8 << "(sp)" << std::endl;
        out << "    addi sp, sp, "<< fp << std::endl;
        out << "    jr ra" << std::endl;
        out << "    .size   main, .-main" << std::endl;
    }else if(!ismain) {  //有系统函数   有自定义函数

        fun_asmCodeGen(out); 

    }

    out << "    .ident  \"GCC: (GNU) 8.3.0\"" << std::endl;
    std::cout << "Print Succeeded!" << std::endl << std::endl << "Closing Complier..." << std::endl;
    return ;
}



void riscvGenerator::nofun_asmCodeGen(std::ofstream &out, Parser &p) {

    //第零项为main函数
    for(unsigned int i = 1;  i < itgenerator.getIntermediateList().size(); i ++) {

        FourYuanInstr fy = itgenerator.getIntermediateList()[i];
        std::string name;
        itemType it;
        int  isglobal = -1;  // 1 Global  0 local  -1 不存在
        int tmp;  //临时计数器
        SymbolItem *head, *tail;
        switch (fy.getopcode()) {
            case PrintStr:
                name = fy.gettarget();
                out << "    lui a5,%hi(" << name << ")" << std::endl;            
                out << "    addi    a0, a5,%lo(" << name << ")" << std::endl;
                out << "    call printf" << std::endl;
                break;
            case PrintId:
                it = fy.getparat();
                name = fy.gettarget();
                //判断ID是全局还是局部变量
                head =  p.getSymbolTable()->getHead();
                tail =  p.getSymbolTable()->getTail();
                while(head != tail) {
                    if(head->getname() == name && head->getSt() == st_localType) {
                        LocalItem *li = static_cast<LocalItem *>(head);
                        it = li->getIt();
                        if(head->getscope() == "Global"){
                            isglobal = 1; 
                        }else {
                            isglobal = 0;
                        }
                        break;
                    } 
                    head = head->next; 
                } 
                if(head == tail && head->getname() == name && head->getSt() == st_localType) {
                    LocalItem *li = static_cast<LocalItem *>(head);
                    it = li->getIt();
                    if(head->getscope() == "Global") {
                        isglobal = 1;
                    }else {
                        isglobal = 0; 
                    }
                }           
                if(isglobal == 1) {   //全局变量   标签寻址
                    if(it == it_intType) {
                        out << "    lui a5, %hi(" << name << ")" << std::endl;
                        out << "    lw a5, %lo(" << name << ")(a5)"  << std::endl;
                        out << "    mv a1, a5" << std::endl;
                        out << "    lui a5, %hi(.PL0)" << std::endl; 
                        out << "    addi a0, a5, %lo(.PL0)" << std::endl;
                        out << "    call printf" << std::endl;
                    }else if(it == it_charType) {
                        out << "    lui a5, %hi(" << name << ")" << std::endl;
                        out << "    lbu a5, %lo(" << name << ")(a5)"  << std::endl;
                        out << "    mv a0, a5" << std::endl;
                        out << "    call putchar" << std::endl;                         
                    }

                }else if(isglobal == 0) {  //main中的局部变量  在栈中寻址
                    
                    head = p.getSymbolTable()->getHead();
                    tail = p.getSymbolTable()->getTail();         
                    tmp  = 0;
                    while(head != tail) {
                        if(head->getscope() == "main" && head->getSt() == st_localType) {
                            tmp++;
                            if(head->getname() == name) {
                                break; 
                            } 
                        } 
                        head = head->next; 
                    }
                    if(head == tail && head->getscope() == "main" && head->getSt() == st_localType) {
                        if(head->getname() == name) {
                            tmp++; 
                        } 
                    }
                    tmp = 16 + tmp*4;
                    out << "    lw  a1, -" << tmp << "(s0)" << std::endl;
                    out << "    lui a5, %hi(.PL0)" << std::endl;
                    out << "    addi  a0, a5, %lo(.PL0)"  << std::endl;
                    out << "    call printf" << std::endl;

                }else if(isglobal == -1) {  //表达式


                }


                break;
            case PrintInt:
                out << "    li a1, " << std::stoi(fy.gettarget())<< std::endl;
                out << "    lui a5, %hi(.PL0)" << std::endl; 
                out << "    addi a0, a5, %lo(.PL0)" << std::endl;
                out << "    call printf" << std::endl;
                break;
            case PrintChar:
                out << "    li a0, " << static_cast<int>(fy.gettarget()[0]) << std::endl;
                out << "    call putchar" << std::endl;
                break;
            case ReadInt:
            case ReadChar:
                /*判断变量是全局变量还是局部变量*/
                name = fy.gettarget();
                head = p.getSymbolTable()->getHead();
                tail = p.getSymbolTable()->getTail();
                while(head != tail) {
                    if(head->getname() == name && head->getSt() == st_localType) {
                        if(head->getscope() == "Global") {
                            isglobal = 1;    
                        } else {
                            isglobal = 0; 
                        }
                        break; 
                    }
                    head = head->next; 
                } 
                if(head->getname() == name && head->getSt() == st_localType) {
                    if(head->getscope() == "Global") {
                        isglobal = 1; 
                    }else {
                        isglobal = 0; 
                    } 
                }
                if(isglobal == 1) {  //全局变量
                    out << "    lui a5, %hi(" << name << ")" << std::endl;
                    out << "    addi a1, a5, %lo(" << name << ")" << std::endl;
                    out << "    lui a5, %hi(.PL0)" << std::endl; 
                    out << "    addi a0, a5, %lo(.PL0)" << std::endl;
                    out << "    call scanf" << std::endl;
                }else if(isglobal == 0) {  //局部变量
                   /*
                    * 局部变量在运行栈中完成
                    *   注意压栈的顺序
                    */ 
                    head = p.getSymbolTable()->getHead();
                    tail = p.getSymbolTable()->getTail();         
                    tmp  = 0;
                    while(head != tail) {
                        if(head->getscope() == "main" && head->getSt() == st_localType) {
                            tmp++;
                            if(head->getname() == name) {
                                break; 
                            } 
                        } 
                        head = head->next; 
                    }
                    if(head == tail && head->getscope() == "main" && head->getSt() == st_localType) {
                        if(head->getname() == name) {
                            tmp++; 
                        } 
                    }
                    tmp = 16 + tmp*4;
                    out << "    addi a5, s0, -" << tmp << std::endl;
                    out << "    mv a1, a5" << std::endl;
                    out << "    lui a5, %hi(.PL0)" << std::endl; 
                    out << "    addi a0, a5, %lo(.PL0)" << std::endl;
                    out << "    call scanf" << std::endl;
                }else if(isglobal == -1) {
                    panic("CodeGenError: Wrong Instruction!");
                }
                break;
            case ADD:

                break;
            case SUB:

                break;
            case MUL:

                break;
            case DIV:

                break;
            case NEG:

                break;
            case ASS:

                break;
            case JMP:

                break;
            case LABEL:
                break;
            case JT:
                break;
            case JNT:
                break;
            case GT:
                break;
            case GE:
                break;
            case LT:
                break;
            case LE:
                break;
            case ENQ:
                break;
            case BNE:
                break;
            case FUNDEC:
            case FUNCALL:
            case PARAM:
                panic("CodeGenError: Wrong Instruction!");
                break; 
            default:
                return;                
        } 
    }

}




void riscvGenerator::fun_asmCodeGen(std::ofstream &out) {

    for(unsigned int i = 0; i < itgenerator.getIntermediateList().size(); i ++)  {
        FourYuanInstr fy = itgenerator.getIntermediateList()[i]; 
        std::string name;



    }

}

