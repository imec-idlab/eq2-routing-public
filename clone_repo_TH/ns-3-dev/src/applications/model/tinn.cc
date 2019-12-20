#include "tinn.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

namespace ns3 {

// Computes error.
static float err(const float a, const float b)
{
    return 0.5f * (a - b) * (a - b);
}

// Returns partial derivative of error function.
static float pderr(const float a, const float b)
{
    return a - b;
}

// Computes total error of target to output.
static float toterr(const float* const tg, const float* const o, const int size)
{
    float sum = 0.0f;
    for(int i = 0; i < size; i++)
        sum += err(tg[i], o[i]);
    return sum;
}

// Activation function.
static float act(const float a)
{
    return 1.0f / (1.0f + expf(-a));
}

// Returns partial derivative of activation function.
static float pdact(const float a)
{
    return a * (1.0f - a);
}

// Returns floating point random from 0.0 - 1.0.
static float frand()
{
    return rand() / (float) RAND_MAX;
}


TinyNN::TinyNN(){}
TinyNN::~TinyNN(){}

// Performs back propagation.
void TinyNN::bprop(const float* const in, const float* const tg, float rate)
{
    for(int i = 0; i < this->nhid; i++)
    {
        float sum = 0.0f;
        // Calculate total error change with respect to output.
        for(int j = 0; j < this->nops; j++)
        {
            const float a = pderr(this->o[j], tg[j]);
            const float b = pdact(this->o[j]);
            sum += a * b * this->x[j * this->nhid + i];
            // Correct weights in hidden to output layer.
            this->x[j * this->nhid + i] -= rate * a * b * this->h[i];
        }
        // Correct weights in input to hidden layer.
        for(int j = 0; j < this->nips; j++)
            this->w[i * this->nips + j] -= rate * sum * pdact(this->h[i]) * in[j];
    }
}

// Performs forward propagation.
void TinyNN::fprop(const float* const in)
{
    // Calculate hidden layer neuron values.
    for(int i = 0; i < this->nhid; i++)
    {
        float sum = 0.0f;
        for(int j = 0; j < this->nips; j++)
            sum += in[j] * this->w[i * this->nips + j];
        this->h[i] = act(sum + this->b[0]);
    }
    // Calculate output layer neuron values.
    for(int i = 0; i < this->nops; i++)
    {
        float sum = 0.0f;
        for(int j = 0; j < this->nhid; j++)
            sum += this->h[j] * this->x[i * this->nhid + j];
        this->o[i] = act(sum + this->b[1]);
    }
}

// Randomizes tinn weights and biases.
void TinyNN::wbrand()
{

    for(int i = 0; i < this->nw; i++) this->w[i] = frand() - 0.5f;
    for(int i = 0; i < this->nb; i++) this->b[i] = frand() - 0.5f;
}

// Returns an output prediction given an input.
float* TinyNN::xtpredict(const float* const in)
{
    this->fprop(in);
    return this->o;
}

// Trains a tinn with an input and target output with a learning rate. Returns target to output error.
float TinyNN::xttrain(const float* const in, const float* const tg, float rate)
{
	this->fprop(in);
	this->bprop(in, tg, rate);
    return toterr(tg, this->o, this->nops);
}

// Constructs a tinn with number of inputs, number of hidden neurons, and number of outputs
void TinyNN::xtbuild(const int nips, const int nhid, const int nops)
{
    // Tinn only supports one hidden layer so there are two biases.
    this->nb = 2;
    this->nw = nhid * (nips + nops);
    this->w = (float*) calloc(this->nw, sizeof(*this->w));
    this->x = this->w + nhid * nips;
    this->b = (float*) calloc(this->nb, sizeof(*this->b));
    this->h = (float*) calloc(nhid, sizeof(*this->h));
    this->o = (float*) calloc(nops, sizeof(*this->o));
    this->nips = nips;
    this->nhid = nhid;
    this->nops = nops;
    this->wbrand();
}

// Saves a tinn to disk.
void TinyNN::xtsave(const char* const path)
{
    FILE* const file = fopen(path, "w");
    // Save header.
    fprintf(file, "%d %d %d\n", this->nips, this->nhid, this->nops);
    // Save biases and weights.
    for(int i = 0; i < this->nb; i++) fprintf(file, "%f\n", (double) this->b[i]);
    for(int i = 0; i < this->nw; i++) fprintf(file, "%f\n", (double) this->w[i]);
    fclose(file);
}

// Loads a tinn from disk.
void TinyNN::xtload(const char* const path)
{
    FILE* const file = fopen(path, "r");
    int nips = 0;
    int nhid = 0;
    int nops = 0;
    // Load header.
    fscanf(file, "%d %d %d\n", &nips, &nhid, &nops);
    // Build a new tinn
    // TODO HANS - creation of a new TinyNN should not reference itself
    this->xtbuild(nips, nhid, nops);
    // Load biaes and weights.
    for(int i = 0; i < this->nb; i++) fscanf(file, "%f\n", &this->b[i]);
    for(int i = 0; i < this->nw; i++) fscanf(file, "%f\n", &this->w[i]);
    fclose(file);
}

// Frees object from heap.
void TinyNN::xtfree()
{
    free(this->w);
    free(this->b);
    free(this->h);
    free(this->o);
}

// Prints an array of floats. Useful for printing predictions.
void TinyNN::xtprint(const float* arr, const int size)
{
    for(int i = 0; i < size; i++)
        printf("%f ", (double) arr[i]);
    printf("\n");
}

}
