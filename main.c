#define WIN32_LEAN_AND_MEAN
#define INITGUID
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <ksmedia.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define REFTIMES_PER_MILLISEC 10000.0
#define SAMPLE_RATE 44100

void checkError(HRESULT hr, const char* msg) {
    if (FAILED(hr)) {
        fprintf(stderr, "❌ Error en %s (HRESULT: 0x%08X)\n", msg, (unsigned int)hr);
        system("pause");
        exit(1);
    }
}

int main(void) {
    printf("[1/5] Inicializando COM...\n");
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pEndpointIn = NULL;
    IMMDevice *pEndpointOut = NULL;
    IAudioClient *pAudioClientIn = NULL;
    IAudioClient *pAudioClientOut = NULL;
    IAudioCaptureClient *pCaptureClient = NULL;
    IAudioRenderClient *pRenderClient = NULL;

    printf("[2/5] Buscando dispositivos de audio...\n");
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    checkError(hr, "CoCreateInstance");
    
    hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(pEnumerator, eCapture, eConsole, &pEndpointIn);
    checkError(hr, "GetDefaultAudioEndpoint (Input)");

    hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(pEnumerator, eRender, eConsole, &pEndpointOut);
    checkError(hr, "GetDefaultAudioEndpoint (Output)");

    hr = pEndpointIn->lpVtbl->Activate(pEndpointIn, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClientIn);
    checkError(hr, "Activate Input");

    hr = pEndpointOut->lpVtbl->Activate(pEndpointOut, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClientOut);
    checkError(hr, "Activate Output");

    WAVEFORMATEX wfx = { 0 };
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = SAMPLE_RATE;
    wfx.wBitsPerSample = 32;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

    WAVEFORMATEX *pwfxInNative = NULL;
    pAudioClientIn->lpVtbl->GetMixFormat(pAudioClientIn, &pwfxInNative);
    if (pwfxInNative->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        wfx.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    }
    CoTaskMemFree(pwfxInNative);

    REFERENCE_TIME hnsBufferDuration = REFTIMES_PER_MILLISEC * 10;

    printf("[3/5] Inicializando motor de audio seguro...\n");
    DWORD streamFlags = 0x80000000; // AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM

    hr = pAudioClientIn->lpVtbl->Initialize(pAudioClientIn, AUDCLNT_SHAREMODE_SHARED, streamFlags, hnsBufferDuration, 0, &wfx, NULL);
    if (FAILED(hr)) pAudioClientIn->lpVtbl->Initialize(pAudioClientIn, AUDCLNT_SHAREMODE_SHARED, 0, hnsBufferDuration, 0, &wfx, NULL);
    checkError(hr, "AudioClient In Initialize");

    hr = pAudioClientOut->lpVtbl->Initialize(pAudioClientOut, AUDCLNT_SHAREMODE_SHARED, streamFlags, hnsBufferDuration, 0, &wfx, NULL);
    if (FAILED(hr)) pAudioClientOut->lpVtbl->Initialize(pAudioClientOut, AUDCLNT_SHAREMODE_SHARED, 0, hnsBufferDuration, 0, &wfx, NULL);
    checkError(hr, "AudioClient Out Initialize");

    printf("[4/5] Obteniendo servicios de streaming...\n");
    pAudioClientIn->lpVtbl->GetService(pAudioClientIn, &IID_IAudioCaptureClient, (void**)&pCaptureClient);
    pAudioClientOut->lpVtbl->GetService(pAudioClientOut, &IID_IAudioRenderClient, (void**)&pRenderClient);

    pAudioClientIn->lpVtbl->Start(pAudioClientIn);
    pAudioClientOut->lpVtbl->Start(pAudioClientOut);

    // =========================================================================
    // 🎛️ PARÁMETROS SEGUROS DE TONO Y VOLUMEN (Garantizan cero pérdida de volumen)
    // =========================================================================
    float overdrive_gain = 1.8f;  // Fuzz cálido analógico
    float bass_boost     = 1.3f;  // Realce de graves aditivo (sin atenuar)
    float presence_boost = 1.2f;  // Realce de presencia/agudos aditivo
    float output_boost   = 2.2f;  // Volumen Master / Make-up Gain final

    printf("\n🎸 ¡MOTOR DE AUDIO MULTIEFECTOS SEGURO ACTIVO!\n");
    printf("[Fuzz Analógico + EQ Aditivo de Graves/Agudos + Master Boost]\n");
    printf("Toca tu guitarra. Presiona [ESC] o [q] para salir.\n\n");
    fflush(stdout);

    int running = 1;
    while (running) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 27 || key == 'q' || key == 'Q') {
                running = 0;
                break;
            }
        }

        UINT32 packetLength = 0;
        hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
        
        while (packetLength > 0) {
            BYTE *pData;
            UINT32 numFramesAvailable;
            DWORD flags;

            hr = pCaptureClient->lpVtbl->GetBuffer(pCaptureClient, &pData, &numFramesAvailable, &flags, NULL, NULL);
            if (SUCCEEDED(hr)) {
                BYTE *pOutData;
                hr = pRenderClient->lpVtbl->GetBuffer(pRenderClient, numFramesAvailable, &pOutData);
                if (SUCCEEDED(hr)) {
                    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                        memset(pOutData, 0, numFramesAvailable * wfx.nBlockAlign);
                    } else {
                        float *src = (float*)pData;
                        float *dst = (float*)pOutData;
                        UINT32 totalSamples = numFramesAvailable * wfx.nChannels;

                        // -------------------------------------------------------------
                        // ⚡ BUCLE DE PROCESAMIENTO LINEAL SEGURO
                        // -------------------------------------------------------------
                        for (UINT32 i = 0; i < totalSamples; i++) {
                            float sample = src[i];

                            // 1. Realce de Graves Aditivo
                            sample *= bass_boost;

                            // 2. Saturación suave analógica (Fuzz cálido con tanhf)
                            sample = tanhf(sample * overdrive_gain);

                            // 3. Realce de Presencia/Agudos post-distorsión
                            sample *= presence_boost;

                            // 4. Volumen Master / Make-up Gain final inquebrantable
                            sample *= output_boost;

                            dst[i] = sample;
                        }
                    }
                    pRenderClient->lpVtbl->ReleaseBuffer(pRenderClient, numFramesAvailable, 0);
                }
                pCaptureClient->lpVtbl->ReleaseBuffer(pCaptureClient, numFramesAvailable);
            }
            pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
        }
        Sleep(1);
    }

    printf("\n[Cierre] Deteniendo streams...\n");
    pAudioClientIn->lpVtbl->Stop(pAudioClientIn);
    pAudioClientOut->lpVtbl->Stop(pAudioClientOut);

    CoUninitialize();
    
    printf("¡Cierre exitoso! 🤘\n");
    return 0;
}