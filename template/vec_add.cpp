#include "common.h"

using namespace cl::sycl;

class VecAddBench
{
protected:    
    int* input1 = nullptr;
    int* input2 = nullptr;
    int* output = nullptr;        
    const BenchmarkArgs &args;

public:
  VecAddBench(const BenchmarkArgs &_args) : args(_args) {}
  
  void setup() {      
      // host memory allocation
      input1 = new int[args.problem_size]; // FIXME memory leak, replace with vector
      input2 = new int[args.problem_size];
      output = new int[args.problem_size];
      // input initialization
      for(int i=0; i<args.problem_size; i++){
          input1[i] = 1;
          input2[i] = 2;
          output[i] = 0;
      }
  }

  void run() {    
    buffer<int, 1> input1_buf(input1, range<1>(args.problem_size));
    buffer<int, 1> input2_buf(input2, range<1>(args.problem_size));
    buffer<int, 1> output_buf(output, range<1>(args.problem_size));

    args.device_queue->submit(
        [&](cl::sycl::handler& cgh) {
      auto in1 = input1_buf.get_access<access::mode::read>(cgh);
      auto in2 = input2_buf.get_access<access::mode::read>(cgh);
      auto out = output_buf.get_access<access::mode::write>(cgh);
      cl::sycl::range<1> ndrange {args.problem_size};

      cgh.parallel_for<class VecAddKernel>(ndrange,
        [=](cl::sycl::id<1> gid) 
        {
            out[gid] = in1[gid] + in2[gid];
        });
    });
  }

  bool verify(VerificationSetting &ver) { 
    // TODO FIXME
    return true;
  }
  
};

int main(int argc, char** argv)
{
  BenchmarkApp app(argc, argv);
  app.run<VecAddBench>();  
  return 0;
}
