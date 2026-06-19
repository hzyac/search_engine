// DataCenter.cpp
#include "../include/dataCenter.hpp"
#include <cstdlib>

DataCenter *DataCenter::_pInstance = nullptr;
std::once_flag DataCenter::_onceFlag;

DataCenter *DataCenter::getInstance()
{
    std::call_once(_onceFlag, [](){
        _pInstance = new DataCenter();
        std::atexit([](){
            if(_pInstance)
            {
                delete _pInstance;
                _pInstance = nullptr;
            }
        });
    });
    return _pInstance;
}