#ifndef TINN_H_
#define TINN_H_

namespace ns3 {

class TinyNN {
public:
	TinyNN();
	~TinyNN();
	void bprop(const float* const, const float* const, float);
	void fprop(const float* const);
	void wbrand();
	float* xtpredict(const float* in);
	float xttrain(const float* in, const float* tg, float rate);
	void xtbuild(int nips, int nhid, int nops);
	void xtsave(const char* path);
	void xtload(const char* path);
	void xtfree();
	void xtprint(const float* arr, const int size);

private:
    // All the weights.
    float* w;
    // Hidden to output layer weights.
    float* x;
    // Biases.
    float* b;
    // Hidden layer.
    float* h;
    // Output layer.
    float* o;
    // Number of biases - always two - Tinn only supports a single hidden layer.
    int nb;
    // Number of weights.
    int nw;
    // Number of inputs.
    int nips;
    // Number of hidden neurons.
    int nhid;
    // Number of outputs.
    int nops;
};

}

#endif /* QDEEP_H */
