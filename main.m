clc;clear;close all


fileID=fopen('input.bin','r');
input=fread(fileID,'float32');
fclose(fileID); clear("fileID");
fileID=fopen('output-NONE.bin','r');
outputNONE=fread(fileID,'float32');
fclose(fileID); clear("fileID");
fileID=fopen('output-CPU.bin','r');
outputCPU=fread(fileID,'float32');
fclose(fileID); clear("fileID");

[m, idx] = max(abs(outputCPU - outputNONE));

ind = 1:500000;
figure; hold on
plot(ind, input(ind), 'Color',0.5*[1,1,1]);
plot(ind, outputNONE(ind), '-');
plot(ind, outputCPU(ind), '-.');
if(m > 0.001), plot(275084, outputCPU(275084), 'r*'); end
