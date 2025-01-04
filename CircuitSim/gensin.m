function retval = gensin ()

fr = 0.15;
fr2= 0.102;
ep = exp(-0.005);
n = 500;

%rd = @(x) floor(32768*x)/32768;
rd = @(x) x;

y = zeros(1,n);
y(1)=rd(cos(pi*fr));
y(2)=rd(cos(2*pi*fr));

z = zeros(1,n);
u = zeros(1,n);
h = zeros(1,n);

ep

for it=3:n
  % Goetzel sinewave generation
  y(it) = rd(2*cos(pi*fr)*y(it-1) - y(it-2));
  % Bandpass filter
  z(it) = rd(2*ep*cos(pi*fr2)*z(it-1) - ep*ep*z(it-2) + y(it));
  % All pass modification
  u(it) = rd(-2*ep*cos(pi*fr2)*z(it-1) + ep*ep*z(it)   + z(it-2));
  % Band pass
  h2(it ) =  rd(z(it) - ep*cos(pi*fr2)*z(it-1));
  h(it) = rd((-2*ep*cos(pi*fr2)*y(it-1) + ep*ep*y(it) + y(it-2) + 2*ep*cos(pi*fr2)*h(it-1) - ep*ep*h(it-2)));
end

h2 = h2*(1-ep*ep);

figure(1);
subplot(5,1,1);
plot(1:n,y,'r-');
subplot(5,1,2);
plot(1:n,z,'b-');
subplot(5,1,3);
plot(1:n,u,'b-');
subplot(5,1,4);
plot(1:n,h,'b-');
subplot(5,1,5);
plot(1:n,h2,'b-');

endfunction
