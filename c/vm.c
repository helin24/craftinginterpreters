#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "debug.h"
#include "vm.h"

VM vm;

void initVM() {
  vm.stackSize = 0;
  
  vm.objects = NULL;
  
  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;
}

void endVM() {
  // Free all objects.
  Obj* obj = vm.objects;
  while (obj != NULL) {
    Obj* next = obj->next;
    freeObject(obj);
    obj = next;
  }
  
  free(vm.grayStack);
}

static void push(Value value) {
  vm.stack[vm.stackSize++] = value;
}

static Value pop() {
  return vm.stack[--vm.stackSize];
}

static Value peek(int distance) {
  return vm.stack[vm.stackSize - distance];
}

// TODO: Lots of duplication here.
static bool popNumbers(double* a, double* b) {
  if (vm.stack[vm.stackSize - 1]->type != OBJ_NUMBER) {
    fprintf(stderr, "Right operand must be a number.\n");
    return false;
  }

  if (vm.stack[vm.stackSize - 2]->type != OBJ_NUMBER) {
    fprintf(stderr, "Left operand must be a number.\n");
    return false;
  }

  *b = ((ObjNumber*)pop())->value;
  *a = ((ObjNumber*)pop())->value;
  return true;
}

static bool popBool(bool* a) {
  if (vm.stack[vm.stackSize - 1]->type != OBJ_BOOL) {
    fprintf(stderr, "Operand must be a number.\n");
    return false;
  }
  
  *a = ((ObjBool*)pop())->value;
  return true;
}

static double popNumber(double* a) {
  if (vm.stack[vm.stackSize - 1]->type != OBJ_NUMBER) {
    fprintf(stderr, "Operand must be a number.\n");
    return false;
  }
  
  *a = ((ObjNumber*)pop())->value;
  return true;
}

static void concatenate() {
  ObjString* b = (ObjString*)peek(1);
  ObjString* a = (ObjString*)peek(2);
  
  ObjString* result = newString(NULL, a->length + b->length);
  memcpy(result->chars, a->chars, a->length);
  memcpy(result->chars + a->length, b->chars, b->length);
  pop();
  pop();
  push((Value)result);
}

static void run(ObjFunction* function) {
  // TODO: Hack. Stuff it on the stack so it doesn't get collected.
  push((Value)function);
  
  uint8_t* ip = function->code;
  for (;;) {
    switch (*ip++) {
      case OP_CONSTANT: {
        uint8_t constant = *ip++;
        push(function->constants[constant]);
        break;
      }
        
      case OP_GREATER: {
        double a, b;
        if (!popNumbers(&a, &b)) return;
        push((Value)newBool(a > b));
        break;
      }
        
      case OP_LESS: {
        double a, b;
        if (!popNumbers(&a, &b)) return;
        push((Value)newBool(a < b));
        break;
      }

      case OP_ADD: {
        if (peek(1)->type == OBJ_STRING &&
            peek(2)->type == OBJ_STRING) {
          concatenate();
        } else if (peek(1)->type == OBJ_NUMBER &&
                   peek(2)->type == OBJ_NUMBER) {
          double b = ((ObjNumber*)pop())->value;
          double a = ((ObjNumber*)pop())->value;
          push((Value)newNumber(a + b));
        } else {
          fprintf(stderr, "Can only add two strings or two numbers.\n");
          return;
        }
        break;
      }
        
      case OP_SUBTRACT: {
        double a, b;
        if (!popNumbers(&a, &b)) return;
        push((Value)newNumber(a - b));
        break;
      }
        
      case OP_MULTIPLY: {
        double a, b;
        if (!popNumbers(&a, &b)) return;
        push((Value)newNumber(a * b));
        break;
      }
        
      case OP_DIVIDE: {
        double a, b;
        if (!popNumbers(&a, &b)) return;
        push((Value)newNumber(a / b));
        break;
      }
        
      case OP_NOT: {
        bool a;
        if (!popBool(&a)) return;
        push((Value)newBool(!a));
        break;
      }

      case OP_NEGATE: {
        double a;
        if (!popNumber(&a)) return;
        push((Value)newNumber(-a));
        break;
      }
        
      case OP_RETURN:
        //printValue(vm->stack[vm->stackSize - 1]);
        return;
    }
  }
}

void interpret(const char* source) {
  ObjFunction* function = compile(source);
  if (function == NULL) return;

  printFunction(function);
  run(function);
  collectGarbage();
  printStack();
}
