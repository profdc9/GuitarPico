% http://shepazu.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html

function retval = gensin2 ()

fext = 0.05;
f0 = 0.1;
Q = 10;
A = sqrt(2.0);

n = 500;

%rd = @(x) floor(32768*x)/32768;
rd = @(x) x;

a = sin(2*pi*f0)/(2*Q);

sinea0 = 1;
sinea1 = -2*cos(2*pi*fext);
sinea2 = 1;
s(1)=rd(cos(2*pi*fext));
s(2)=rd(cos(4*pi*fext));

lpf = zeros(1,n);
lpfb0 = (1-cos(2*pi*f0))/2;
lpfb1 = (1-cos(2*pi*f0));
lpfb2 = (1-cos(2*pi*f0))/2;
lpfa0 = 1+a;
lpfa1 = -2*cos(2*pi*f0);
lpfa2 = 1-a;

hpf = zeros(1,n);
hpfb0 = (1+cos(2*pi*f0))/2;
hpfb1 = -(1+cos(2*pi*f0));
hpfb2 = (1+cos(2*pi*f0))/2;
hpfa0 = 1+a;
hpfa1 = -2*cos(2*pi*f0);
hpfa2 = 1-a;

bpf = zeros(1,n);
bpfb0 = a;
bpfb1 = 0;
bpfb2 = -a;
bpfa0 = 1+a;
bpfa1 = -2*cos(2*pi*f0);
bpfa2 = 1-a;

apf = zeros(1,n);
apfb0 = 1-a;
apfb1 = -2*cos(2*pi*f0);
apfb2 = 1+a;
apfa0 = 1+a;
apfa1 = -2*cos(2*pi*f0);
apfa2 = 1-a;

peakf = zeros(1,n);
peakfb0 = 1+a*A;
peakfb1 = -2*cos(2*pi*f0);
peakfb2 = 1-a*A;
peakfa0 = 1+a/A;
peakfa1 = -2*cos(2*pi*f0);
peakfa2 = 1-a/A;

lowshelff = zeros(1,n);
lowshelffb0 = A*((A+1)-(A-1)*cos(2*pi*f0)+2*sqrt(A)*a);
lowshelffb1 = 2*A*((A-1)-(A+1)*cos(2*pi*f0));
lowshelffb2 = A*((A+1)-(A-1)*cos(2*pi*f0)-2*sqrt(A)*a);
lowshelffa0 = (A+1)+(A-1)*cos(2*pi*f0)+2*sqrt(A)*a;
lowshelffa1 = -2*((A-1)+(A+1)*cos(2*pi*f0));
lowshelffa2 = (A+1)+(A-1)*cos(2*pi*f0)-2*sqrt(A)*a;

highshelff = zeros(1,n);
highshelffb0 = A*((A+1)+(A-1)*cos(2*pi*f0)+2*sqrt(A)*a);
highshelffb1 = -2*A*((A-1)+(A+1)*cos(2*pi*f0));
highshelffb2 = A*((A+1)+(A-1)*cos(2*pi*f0)-2*sqrt(A)*a);
highshelffa0 = (A+1)-(A-1)*cos(2*pi*f0)+2*sqrt(A)*a;
highshelffa1 = 2*((A-1)-(A+1)*cos(2*pi*f0));
highshelffa2 = (A+1)-(A-1)*cos(2*pi*f0)-2*sqrt(A)*a;

for it=3:n
  % Goetzel sinewave generation
  s(it) = rd(-(sinea1/sinea0)*s(it-1)-(sinea2/sinea0)*s(it-2));
  % Low-pass filter
  lpf(it) = rd((lpfb0/lpfa0)*s(it) + (lpfb1/lpfa0)*s(it-1) + (lpfb2/lpfa0)*s(it-2) - (lpfa1/lpfa0)*lpf(it-1) - (lpfa2/lpfa0)*lpf(it-2));
  % High-pass filter
  hpf(it) = rd((hpfb0/hpfa0)*s(it) + (hpfb1/hpfa0)*s(it-1) + (hpfb2/hpfa0)*s(it-2) - (hpfa1/hpfa0)*hpf(it-1) - (hpfa2/hpfa0)*hpf(it-2));
  % Band-pass filter
  bpf(it) = rd((bpfb0/bpfa0)*s(it) + (bpfb1/bpfa0)*s(it-1) + (bpfb2/bpfa0)*s(it-2) - (bpfa1/bpfa0)*bpf(it-1) - (bpfa2/bpfa0)*bpf(it-2));
  % All-pass filter
  apf(it) = rd((apfb0/apfa0)*s(it) + (apfb1/apfa0)*s(it-1) + (apfb2/apfa0)*s(it-2) - (apfa1/apfa0)*apf(it-1) - (apfa2/apfa0)*apf(it-2));
  % Peaking EQ filter
  peakf(it) = rd((peakfb0/peakfa0)*s(it) + (peakfb1/peakfa0)*s(it-1) + (peakfb2/peakfa0)*s(it-2) - ...
                 (peakfa1/peakfa0)*peakf(it-1) - (peakfa2/peakfa0)*peakf(it-2));
  % Low Shelf Filter
  lowshelff(it) = rd((lowshelffb0/lowshelffa0)*s(it) + (lowshelffb1/lowshelffa0)*s(it-1) + (lowshelffb2/lowshelffa0)*s(it-2) - ...
                     (lowshelffa1/lowshelffa0)*lowshelff(it-1) - (lowshelffa2/lowshelffa0)*lowshelff(it-2));
  % High Shelf Filter
  highshelff(it) = rd((highshelffb0/highshelffa0)*s(it) + (highshelffb1/highshelffa0)*s(it-1) + (highshelffb2/highshelffa0)*s(it-2) - ...
                      (highshelffa1/highshelffa0)*highshelff(it-1) - (highshelffa2/highshelffa0)*highshelff(it-2));
end

figure(1);
subplot(4,2,1);
plot(1:n,s,'b-');
title('Sine wave');
subplot(4,2,2);
plot(1:n,lpf,'b-');
title('Low pass filter');
subplot(4,2,3);
plot(1:n,hpf,'b-');
title('High pass filter');
subplot(4,2,4);
plot(1:n,bpf,'b-');
title('Band pass filter');
subplot(4,2,5);
plot(1:n,apf,'b-');
title('All pass filter');
subplot(4,2,6);
plot(1:n,peakf,'b-');
title('Peak Filter');
subplot(4,2,7);
plot(1:n,lowshelff,'b-');
title('Low Shelf Filter');
subplot(4,2,8);
plot(1:n,highshelff,'b-');
title('High Shelf Filter');


endfunction
