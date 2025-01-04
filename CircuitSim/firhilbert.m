## Copyright (C) 2022 Daniel Marks
##
## This is licensed under the zlib license 
## 
## Permission is granted to anyone to use this software for any purpose,#
## including commercial applications, and to alter it and redistribute it
## freely, subject to the following restrictions:
## 
## 1. The origin of this software must not be misrepresented; you must not
##   claim that you wrote the original software. If you use this software
##   in a product, an acknowledgment in the product documentation would be
##   appreciated but is not required.
## 2. Altered source versions must be plainly marked as such, and must not be
##   misrepresented as being the original software.
## 3. This notice may not be removed or altered from any source distribution.
## Author: Daniel Marks <Daniel Marks@VECTRON>
## Created: 2022-01-24

## Simple hilbert FIR coefficient filter for odd n
## n = number of taps
## bits = quantized to bits bits
## coswindow = cosine window applied (0.0 to 1.0)
##    0=rectangular, 1=cosine, 0.5=Hann, 0.53836=Hamming
## coswindowpwr = power of cosine window (normally one)

function qcoeffs = firhilbert (n,bits,coswindow,coswindowpwr)

if nargin<4
    coswindowpwr = 1;
  endif
if nargin<3
    coswindow = 0.0
  endif

# Make n odd
n=floor(n/2)*2+1;
samp = (-n:n);
coeffs = 1./(samp).*(mod(samp,2)==1);
coeffs(n+1)=0;
coswindow = coswindow*(cos(((pi/2)/(n+1)).*samp)).^coswindowpwr+(1.0-coswindow);
coeffs = coeffs.*coswindow;

coeffs = coeffs.*(2/(pi*max(abs(coeffs))));
qcoeffs = floor(coeffs * 2^bits + 0.5);
coeffs = qcoeffs/(2^bits);


figure(1);
resp = [ coeffs(n+1:2*n+1) zeros(1,2*n-1) coeffs(1:n) ];
spectrum = fftshift(fft(resp));

nyq = (-2*n:2*n-1)/(4*n);
subplot(2,2,1);
plot(samp,coeffs,'.')';
xlabel('Sample #');
ylabel('Value');
subplot(2,2,3);
plot(nyq,20*log10(abs(spectrum)),'r');
axis([-0.5 0.5 -1 1]);
xlabel('Sample Frequency Frac');
ylabel('Amplitude (dB)');
subplot(2,2,4);
plot(nyq,angle(spectrum),'b');
axis([-0.5 0.5 -pi pi]);
xlabel('Sample Frequency Frac');
ylabel('Phase (rad)');

endfunction
