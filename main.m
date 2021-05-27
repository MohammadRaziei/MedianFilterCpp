clc;clear;close all


fileID=fopen('input.bin','r');
input=fread(fileID,'float32');
fclose(fileID); clear("fileID");

fileID=fopen('output-meanNONE.bin','r');
output_meanNONE=fread(fileID,'float32');
fclose(fileID); clear("fileID");

fileID=fopen('output-meanCPU.bin','r');
output_meanCPU=fread(fileID,'float32');
fclose(fileID); clear("fileID");

fileID=fopen('output-medianNONE.bin','r');
output_medianNONE=fread(fileID,'float32');
fclose(fileID); clear("fileID");

fileID=fopen('output-medianCPU.bin','r');
output_medianCPU=fread(fileID,'float32');
fclose(fileID); clear("fileID");

ind = 1:500000;


[m, idx] = max(abs(output_meanCPU - output_meanNONE));
figure; hold on
title('Moving-Average Filter')
plot(ind, input(ind), 'Color',0.5*[1,1,1]);
plot(ind, output_meanNONE(ind), '-');
plot(ind, output_meanCPU(ind), '-.');
if(m > 0.001), plot(idx, output_meanCPU(idx), 'r*'); end
legend('Input', 'NONE', 'CPU');

figure; hold on
title('Moving-Average Filter')
% plot(ind, input(ind), 'Color',0.5*[1,1,1]);
plot(ind, output_meanCPU(ind) - output_meanNONE(ind), '.-');
% ylim([-1 1]*10e-2)


% 
[m, idx] = max(abs(output_medianCPU - output_medianNONE));
figure; hold on
title('Median Filter')
plot(ind, input(ind), 'Color',0.5*[1,1,1]);
plot(ind, output_medianNONE(ind), '-');
plot(ind, output_medianCPU(ind), '-.');
if(m > 1e-4), plot(idx, output_medianCPU(idx), 'r*'); end
legend('Input', 'NONE', 'CPU');

figure; hold on
title('Median Filter')
plot(ind, output_medianCPU(ind) - output_medianNONE(ind), '.-');

figure; hold on
title('Compare Filters')
plot(ind, input(ind), 'Color',0.5*[1,1,1]);
plot(ind, output_medianNONE(ind), '-');
plot(ind, output_meanNONE(ind), '-.');
legend('Input', 'Median', 'Moving-Average');
