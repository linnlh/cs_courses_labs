(*
 *  CS164 Fall 94
 *
 *  Programming Assignment 1
 *    Implementation of a simple stack machine.
 *
 *  Skeleton file
 *)

class Main inherits IO {
   st : Stack <- new Stack;
   str : String;
   cmd : StackCommand;

   main() : Object {{
      out_string("> ");
      str <- in_string();
      while not str = "x" loop {
         parse_cmd(str);
         cmd.run(st, str);
         out_string("> ");
         str <- in_string();
      }
      pool;
   }};

   parse_cmd(str : String) : StackCommand {{
      if str = "d"
      then {
         cmd <- new DisplayCmd;
      }
      else {
         if str = "e"
         then {
            cmd <- new EvaluateCmd;
         }
         else {
            cmd <- new PushCmd;
         }
         fi;
      }
      fi;
      cmd;
   }};
};

class Stack {
   size_ : Int <- 0;
   head_ : Node <- (new Node);

   empty() : Bool { size_ = 0 };

   push(str : String) : Object {{
      let node : Node <- (new Node).init(str, head_.next()) in {
         head_.set_next(node);
         size_ <- size_ + 1;
      };
   }};

   pop() : String {{
      if size_ = 0
      then {
         abort();
         "";
      } else {
         let node : Node <- head_.next() in {
            head_.set_next(node.next());
            size_ <- size_ - 1;
            node.value();
         };
      } fi;
   }};

   top() : String {{
      if size_ = 0
      then {
         abort();
         "";
      } else {
         head_.next().value();
      } fi;
   }};

   head() : Node { head_ };
   size() : Int { size_ };
};

class Node {
   value_ : String;
   next_ : Node;

   init(v : String, n : Node) : Node {{
      value_ <- v;
      next_  <- n;
      self;
   }};

   value() : String  { value_ };
   next()  : Node { next_  };
   set_value(str : String) : Object { value_ <- str };
   set_next(n : Node) : Object { next_ <- n };
};

class StackCommand {
   run(st : Stack, str : String) : Object {
      (new IO).out_string("Unimplemented!\n")
   };
};

class PushCmd inherits StackCommand {
   run(st : Stack, str : String) : Object {{
      st.push(str);
   }};
};

class EvaluateCmd inherits StackCommand {
   v1 : String;
   v2 : String;

   run(st : Stack, str : String) : Object {{
      if not st.empty()
      then {
         let str : String <- st.pop() in {
            if str = "+"
            then {
               v1 <- st.pop();
               v2 <- st.pop();
               let v1_int : Int <- (new A2I).a2i(v1), v2_int : Int <- (new A2I).a2i(v2) in {
                  st.push((new A2I).i2a(v1_int + v2_int));
               };
            }
            else {
               if str = "s"
               then {
                  v1 <- st.pop();
                  v2 <- st.pop();
                  st.push(v1);
                  st.push(v2);
               }
               else {
                  st.push(str);
                  0;
               }
               fi;
            }  
            fi;
         };
      }
      else {
         0;
      }
      fi;
   }};
};

class DisplayCmd inherits StackCommand {
   run(st : Stack, str : String) : Object {{
      let io : IO <- new IO, node : Node <- st.head().next() in {
         while not isvoid node loop {
            io.out_string(node.value().concat("\n"));
            node <- node.next();
         } pool;
      };
   }};
};