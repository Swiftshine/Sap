#include <telkin/Print.h>
#include <sap/Sap.h>

red::Registrar* sap::getRegistrar() {
    static red::Registrar sRegistrar("sap");
    return &sRegistrar;
}

void main() {
    tk::println("Welcome to Swiftshine's Actor Pack!");
}
